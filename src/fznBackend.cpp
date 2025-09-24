#include "atlantis/fznBackend.hpp"

#include <fznparser/parser.hpp>
#include <thread>
#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchController.hpp"
#include "atlantis/search/threadController.hpp"
#include "atlantis/solverThread.hpp"
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
    const invariantgraph::SolverMapping& mapping,
    const search::Assignment& assignment) {
  for (const auto& outputVar : invariantGraph.outputBoolVars(mapping)) {
    printBoolVar(assignment, outputVar);
  }
  for (const auto& outputVar : invariantGraph.outputIntVars(mapping)) {
    printIntVar(assignment, outputVar);
  }
  for (const auto& outputVarArray : invariantGraph.outputBoolVarArrays(mapping)) {
    printBoolVarArray(assignment, outputVarArray);
  }
  for (const auto& outputVarArray : invariantGraph.outputIntVarArrays(mapping)) {
    printIntVarArray(assignment, outputVarArray);
  }

  std::cout << "----------\n";
}

search::SavedAssignment FznBackend::onSolutionDefault(
    const invariantgraph::FznInvariantGraph& invariantGraph,
    const invariantgraph::SolverMapping& mapping,
    const search::Assignment& assignment, search::ThreadController& controller,
    const Int threadId) {
  auto savedAssignment = search::SavedAssignment(assignment);
  savedAssignment = controller.trySolution(threadId, savedAssignment);

  if (savedAssignment.getCost().getObjective() ==
          assignment.getCost().getObjective() &&
      controller.shouldPrint(threadId)) {
    displaySolution(invariantGraph, mapping, assignment);
    controller.hasPrinted();
  }

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

void FznBackend::solve(logging::Logger& logger) {
  // Shared data
  fznparser::ProblemType problemType = _model->solveType().problemType();
  const auto objectiveDirection = getObjectiveDirection(problemType);
  auto schedule = _annealingScheduleFactory.create();

  auto controller = std::make_shared<search::ThreadController>();

  std::vector<std::thread> threads;
  std::cerr << "Thread count is " << _threadCount << "\n";

  invariantgraph::FznInvariantGraph invariantGraph(true);
  invariantGraph.open();
  logger.timedProcedure("building invariant graph",
                        [&] { invariantGraph.build(*_model); });
  invariantGraph.close();


  for (std::uint_fast32_t threadId = 0; threadId < _threadCount; threadId++) {
    threads.emplace_back([&invariantGraph, &logger, &objectiveDirection, &problemType, &schedule,
                          threadId, &controller, this] {
      auto thread =
          SolverThread(objectiveDirection, problemType, schedule, threadId,
                       controller, _model, _seed + threadId, _dotFilePath,
                       _timelimit, _onSolution, _onFinish);
      _dotFilePath.reset();  // InvariantGraph will only be saved once
      thread.solve(invariantGraph, logger);
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  if (controller->getBestThreadId() >= 0) {
    std::cerr << "Best result is " << controller->getCost().toString()
              << " from thread " << controller->getBestThreadId() << std::endl;
  } else {
    std::cerr << "No solution found!" << std::endl;
  }
}

}  // namespace atlantis
