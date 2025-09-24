#include "atlantis/solverThread.hpp"

#include <fstream>
#include <fznparser/parser.hpp>
#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchController.hpp"
#include "atlantis/search/searchProcedure.hpp"
#include "atlantis/search/searchStatistics.hpp"
#include "atlantis/types.hpp"

namespace atlantis {

void SolverThread::saveInvariantGraph(
    const invariantgraph::FznInvariantGraph& invariantGraph) const {
  std::ofstream dotFile;
  dotFile.open(*_dotFilePath);
  if (dotFile) {
    invariantGraph.writeDotFile(dotFile);
  }
  dotFile.close();
}
void SolverThread::solve(logging::Logger& logger) {
  propagation::Solver solver;

  // TODO: we should improve the initialisation in order to avoid the need for
  // breaking the dynamic cycles
  invariantgraph::FznInvariantGraph invariantGraph(solver, true);
  logger.timedProcedure("building invariant graph",
                        [&] { invariantGraph.build(*_model); });
  invariantGraph.construct();

  if (_dotFilePath.has_value()) saveInvariantGraph(invariantGraph);

  auto neighborhood = invariantGraph.neighborhood();
  neighborhood.printNeighborhood(logger);

  // Might be changed to shared later
  search::Objective searchObjective(solver, _problemType);

  auto violation = searchObjective.registerNode(
      invariantGraph.totalViolationVarId(), invariantGraph.objectiveVarId());

  invariantGraph.close();

  // TODO: extract to shared -- requires the original invariantGraph
  const Int objectiveOptimalValue =
      _model->isSatisfactionProblem() ? 0
      : _model->isMinimisationProblem()
          ? invariantGraph.objectiveVarNode().lowerBound()
          : invariantGraph.objectiveVarNode().upperBound();

  search::Assignment assignment(solver, neighborhood, violation,
                                invariantGraph.objectiveVarId(),
                                _objectiveDirection, objectiveOptimalValue);

  // This can possibly be extracted, or restricted to one thread
  // TODO: this case may not be handled properly
  if (neighborhood.coveredVars().empty()) {
    search::SavedAssignment savedAssignment =
        _onSolution(invariantGraph, assignment, *_controller, _threadId);
    _onFinish(true);
  }

  // Initialize thread-dependent stuff
  logger.debug("Thread {} Using seed {}.", _threadId, _seed);
  search::RandomProvider random(_seed);
  search::Annealer annealer(random, *_schedule, assignment);
  search::SearchProcedure search(random, assignment, neighborhood,
                                 searchObjective, _searchType);

  // TODO: extract to shared -- requires fixing invariantGraph
  auto onSolution = [&](const search::Assignment& a) {
    return _onSolution(invariantGraph, a, *_controller, _threadId);
  };
  auto onFinish = [&](const bool hadSol) { _onFinish(hadSol); };
  search::SearchController searchController(_model->isSatisfactionProblem(),
                                            std::move(onSolution),
                                            std::move(onFinish), _timelimit);

  logger.timedFunction<int>(
      "search", [&] { return search.run(searchController, annealer, logger); });
}

}  // namespace atlantis