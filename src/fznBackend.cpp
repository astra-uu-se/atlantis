#include "atlantis/fznBackend.hpp"

#include <fznparser/parser.hpp>
#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/searchProcedure.hpp"
#include "atlantis/utils/fznOutput.hpp"

namespace atlantis {

std::string toIntString(const search::Assignment& assignment,
                        const std::variant<propagation::VarViewId, Int>& var) {
  return std::to_string(
      std::holds_alternative<Int>(var)
          ? std::get<Int>(var)
          : assignment.value(std::get<propagation::VarViewId>(var)));
}

std::string toBoolString(const search::Assignment& assignment,
                         const std::variant<propagation::VarViewId, Int>& var) {
  return ((std::holds_alternative<Int>(var)
               ? std::get<Int>(var)
               : assignment.value(std::get<propagation::VarViewId>(var))) == 0)
             ? "true"
             : "false";
}

void printBoolVar(const search::Assignment& assignment,
                  const FznOutputVar& outputVar) {
  std::cout << outputVar.identifier << " = "
            << toBoolString(assignment, outputVar.var) << ";\n";
}

void printIntVar(const search::Assignment& assignment,
                 const FznOutputVar& outputVar) {
  std::cout << outputVar.identifier << " = "
            << toIntString(assignment, outputVar.var) << ";\n";
}

std::string arrayVarPrefix(const std::vector<Int>& indexSetSizes) {
  std::string s = " = array" + std::to_string(indexSetSizes.size()) + "d(";

  for (Int size : indexSetSizes) {
    s += "1.." + std::to_string(size) + ", ";
  }

  return s;
}

void printBoolVarArray(const search::Assignment& assignment,
                       const FznOutputVarArray& varArray) {
  std::cout << varArray.identifier << arrayVarPrefix(varArray.indexSetSizes)
            << '[';

  for (size_t i = 0; i < varArray.vars.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    std::cout << toBoolString(assignment, varArray.vars[i]);
  }

  std::cout << "]);\n";
}

void printIntVarArray(const search::Assignment& assignment,
                      const FznOutputVarArray& varArray) {
  std::cout << varArray.identifier << arrayVarPrefix(varArray.indexSetSizes)
            << '[';

  for (size_t i = 0; i < varArray.vars.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    std::cout << toIntString(assignment, varArray.vars[i]);
  }

  std::cout << "]);\n";
}

void FznBackend::onSolutionDefault(
    const invariantgraph::FznInvariantGraph& invariantGraph,
    const search::Assignment& assignment) {
  for (const auto& outputVar : invariantGraph.outputBoolVars()) {
    printBoolVar(assignment, outputVar);
  }
  for (const auto& outputVar : invariantGraph.outputIntVars()) {
    printIntVar(assignment, outputVar);
  }
  for (const auto& outputVarArray : invariantGraph.outputBoolVarArrays()) {
    printBoolVarArray(assignment, outputVarArray);
  }
  for (const auto& outputVarArray : invariantGraph.outputIntVarArrays()) {
    printIntVarArray(assignment, outputVarArray);
  }

  std::cout << "----------\n";
}

void FznBackend::onFinishDefault(bool hadSol) {
  if (!hadSol) {
    std::cout << "=====UNKNOWN=====\n";
  }
}

FznBackend::FznBackend(logging::Logger& logger,
                       std::filesystem::path&& modelFile)
    : FznBackend(
          logger.timedFunction<fznparser::Model>("parsing FlatZinc", [&] {
            auto m = fznparser::parseFznFile(modelFile);
            logger.debug("Found {:d} variable(s)", m.vars().size());
            logger.debug("Found {:d} constraint(s)", m.constraints().size());
            return m;
          })) {}

static propagation::ObjectiveDirection getObjectiveDirection(
    fznparser::ProblemType problemType) {
  switch (problemType) {
    case fznparser::ProblemType::MINIMIZE:
      return propagation::ObjectiveDirection::MINIMIZE;
    case fznparser::ProblemType::MAXIMIZE:
      return propagation::ObjectiveDirection::MAXIMIZE;
    case fznparser::ProblemType::SATISFY:
    default:
      return propagation::ObjectiveDirection::NONE;
  }
}

search::SearchStatistics FznBackend::solve(logging::Logger& logger) {
  fznparser::ProblemType problemType = _model.solveType().problemType();

  propagation::Solver solver;

  // TODO: we should improve the initialisation in order to avoid the need for
  // breaking the dynamic cycles
  invariantgraph::FznInvariantGraph invariantGraph(solver, true);
  logger.timedProcedure("building invariant graph",
                        [&] { invariantGraph.build(_model); });

  invariantGraph.construct();
  if (_dotFilePath.has_value()) {
    std::ofstream dotFile;
    dotFile.open(*_dotFilePath);
    if (dotFile) {
      invariantGraph.writeDotFile(dotFile);
    }
    dotFile.close();
  }
  auto neighbourhood = invariantGraph.neighbourhood();

  neighbourhood.printNeighbourhood(logger);

  search::Objective searchObjective(solver, problemType);

  auto violation = searchObjective.registerNode(
      invariantGraph.totalViolationVarId(), invariantGraph.objectiveVarId());

  invariantGraph.close();

  const Int objectiveOptimalValue =
      _model.isSatisfactionProblem()
          ? 0
          : (_model.isMinimisationProblem()
                 ? invariantGraph.objectiveVarNode().lowerBound()
                 : invariantGraph.objectiveVarNode().upperBound());

  search::Assignment assignment(
      solver, violation, invariantGraph.objectiveVarId(),
      getObjectiveDirection(problemType), objectiveOptimalValue);

  if (neighbourhood.coveredVars().empty()) {
    _onSolution(invariantGraph, assignment);
    _onFinish(true);
    return search::SearchStatistics{};
  }

  logger.debug("Using seed {}.", _seed);
  search::RandomProvider random(_seed);

  search::SearchProcedure search(random, assignment, neighbourhood,
                                 searchObjective);

  std::function<void(const search::Assignment&)> onSolution =
      [&](const search::Assignment& assignment) {
        _onSolution(invariantGraph, assignment);
      };
  std::function<void(bool)> onFinish = [&](bool hadSol) { _onFinish(hadSol); };

  search::SearchController searchController(_model.isSatisfactionProblem(),
                                            std::move(onSolution),
                                            std::move(onFinish), _timelimit);

  auto schedule = _annealingScheduleFactory.create();
  search::Annealer annealer(assignment, random, *schedule);

  return logger.timedFunction<search::SearchStatistics>(
      "search", [&] { return search.run(searchController, annealer, logger); });
}

}  // namespace atlantis
