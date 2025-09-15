#include "atlantis/fznBackend.hpp"

#include <fznparser/parser.hpp>
#include <thread>
#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchController.hpp"
#include "atlantis/search/searchProcedure.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/search/threadController.hpp"
#include "atlantis/utils/fznOutput.hpp"

namespace atlantis {

std::string toIntString(const search::Assignment& assignment,
                        const std::variant<propagation::VarViewId, Int>& var) {
  return std::to_string(
      std::holds_alternative<Int>(var)
          ? std::get<Int>(var)
          : assignment.committedValue(std::get<propagation::VarViewId>(var)));
}

std::string toBoolString(const search::Assignment& assignment,
                         const std::variant<propagation::VarViewId, Int>& var) {
  return ((std::holds_alternative<Int>(var)
               ? std::get<Int>(var)
               : assignment.committedValue(
                     std::get<propagation::VarViewId>(var))) == 0)
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

  for (const Int size : indexSetSizes) {
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

void FznBackend::displaySolution(
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

search::SavedAssignment FznBackend::onSolutionDefault(
    const invariantgraph::FznInvariantGraph& invariantGraph,
    const search::Assignment& assignment, search::ThreadController& controller,
    Int threadId) {
  // TODO: this saved assignment is only used to pass a cost.
  // This should either be used more or replaced by a search::Cost.
  search::SavedAssignment savedAssignment = search::SavedAssignment(assignment);
  search::Cost bestCost =
      controller.trySolution(threadId, savedAssignment.getCost());

  if (bestCost.getObjective() == assignment.getCost().getObjective()) {
    displaySolution(invariantGraph, assignment);
  }

  savedAssignment.setCost(bestCost);
  return savedAssignment;
}

void FznBackend::onFinishDefault(bool hadSol) {
  if (!hadSol) {
    std::cout << "=====UNKNOWN=====\n";
  }
}

FznBackend::FznBackend(logging::Logger& logger,
                       std::filesystem::path&& modelFile,
                       const uint_fast32_t threadCount)
    : FznBackend(logger.timedFunction<fznparser::Model>(
                     "parsing FlatZinc",
                     [&] {
                       auto m = fznparser::parseFznFile(modelFile);
                       logger.debug("Found {:d} variable(s)", m.vars().size());
                       logger.debug("Found {:d} constraint(s)",
                                    m.constraints().size());
                       return m;
                     }),
                 threadCount) {}

static ObjectiveDirection getObjectiveDirection(
    fznparser::ProblemType problemType) {
  switch (problemType) {
    case fznparser::ProblemType::MINIMIZE:
      return ObjectiveDirection::MINIMIZE;
    case fznparser::ProblemType::MAXIMIZE:
      return ObjectiveDirection::MAXIMIZE;
    case fznparser::ProblemType::SATISFY:
    default:
      return ObjectiveDirection::NONE;
  }
}

void FznBackend::solveThread(
    logging::Logger& logger, uint_fast32_t threadId,
    ObjectiveDirection objectiveDirection, fznparser::ProblemType problemType,
    std::shared_ptr<search::AnnealingSchedule> schedule,
    search::ThreadController& controller) {
  propagation::Solver solver;

  // TODO: we should improve the initialisation in order to avoid the need for
  // breaking the dynamic cycles
  invariantgraph::FznInvariantGraph invariantGraph(solver, true);
  logger.timedProcedure("building invariant graph",
                        [&] { invariantGraph.build(_model); });
  invariantGraph.construct();

  // We only want this to happen once
  // Either before the parallelization or only on 1 thread
  if (threadId == 0 && _dotFilePath.has_value()) {
    std::ofstream dotFile;
    dotFile.open(*_dotFilePath);
    if (dotFile) {
      invariantGraph.writeDotFile(dotFile);
    }
    dotFile.close();
  }

  auto neighborhood = invariantGraph.neighborhood();
  neighborhood.printNeighborhood(logger);

  // Might be changed to shared later
  search::Objective searchObjective(solver, problemType);

  auto violation = searchObjective.registerNode(
      invariantGraph.totalViolationVarId(), invariantGraph.objectiveVarId());

  invariantGraph.close();

  // TODO: extract to shared -- requires the original invariantGraph
  const Int objectiveOptimalValue =
      _model.isSatisfactionProblem() ? 0
      : _model.isMinimisationProblem()
          ? invariantGraph.objectiveVarNode().lowerBound()
          : invariantGraph.objectiveVarNode().upperBound();

  search::Assignment assignment(solver, neighborhood, violation,
                                invariantGraph.objectiveVarId(),
                                objectiveDirection, objectiveOptimalValue);

  // This can possibly be extracted, or restricted to one thread
  // TODO: this case isn't handled properly
  if (neighborhood.coveredVars().empty()) {
    // NOTE: this may be the source of the SavedAssignment stats issue
    search::SearchStatistics statistics;
    search::SavedAssignment savedAssignment =
        _onSolution(invariantGraph, assignment, controller, threadId);
    _onFinish(true);
  }

  logger.debug("Using seed {}.", _seed + threadId);
  search::RandomProvider random(_seed + threadId);

  search::SearchProcedure search(random, assignment, neighborhood,
                                 searchObjective);

  search::Annealer annealer(random, *schedule, assignment);

  // TODO: extract to shared -- requires fixing invariantGraph
  auto onSolution = [&](const search::Assignment& a) {
    return _onSolution(invariantGraph, a, controller, threadId);
  };
  auto onFinish = [&](const bool hadSol) { _onFinish(hadSol); };
  search::SearchController searchController(_model.isSatisfactionProblem(),
                                            std::move(onSolution),
                                            std::move(onFinish), _timelimit);

  logger.timedFunction<int>(
      "search", [&] { return search.run(searchController, annealer, logger); });
}

void FznBackend::solve(logging::Logger& logger) {
  // Shared data
  fznparser::ProblemType problemType = _model.solveType().problemType();
  const auto objectiveDirection = getObjectiveDirection(problemType);
  auto schedule = _annealingScheduleFactory.create();

  // This handles communication between threads
  search::ThreadController controller = search::ThreadController();

  std::vector<std::thread> threads;
  std::cerr << "Thread count is " << _threadCount << "\n";

  for (std::uint_fast32_t threadId = 0; threadId < _threadCount; threadId++) {
    threads.emplace_back([&logger, &objectiveDirection, &problemType, &schedule,
                          threadId, &controller, this] {
      solveThread(logger, threadId, objectiveDirection, problemType, schedule,
                  controller);
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  if (controller.getBestThreadId() >= 0) {
    std::cerr << "Best result is " << controller.getCost().toString()
              << " from thread " << controller.getBestThreadId() << std::endl;
  } else {
    std::cerr << "No solution found!" << std::endl;
  }
}

}  // namespace atlantis
