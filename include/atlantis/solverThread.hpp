#pragma once

#include <filesystem>
#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/objective.hpp"
#include "search/annealing/annealingScheduleFactory.hpp"
#include "search/savedAssignment.hpp"
#include "search/searchProcedure.hpp"
#include "search/threadController.hpp"
#include "types.hpp"

namespace atlantis {

class SolverThread {
  std::shared_ptr<const invariantgraph::FznInvariantGraph> _invariantGraph;
  std::vector<invariantgraph::VarNodeId> _outputVarNodeIds;
  const search::AnnealingScheduleFactory& _annealingScheduleFactory;
  fznparser::ProblemType _problemType;
  size_t _threadId;
  std::shared_ptr<search::ThreadController> _threadController;
  search::SearchType _searchType;

  // Optional arguments
  std::uint_fast32_t _seed;
  std::optional<std::chrono::milliseconds> _timelimit;
  std::shared_ptr<const bool> _shouldStop;

  std::function<void(const search::SavedAssignment&, search::ThreadController&,
                     Int threadId)>
      _onSolution;

  std::function<void(bool)> _onFinish;

 public:
  explicit SolverThread(
      const std::shared_ptr<invariantgraph::FznInvariantGraph>& invariantGraph,
      std::vector<invariantgraph::VarNodeId>&& outputVarNodeIds,
      const search::AnnealingScheduleFactory& annealingScheduleFactory,
      const fznparser::ProblemType problemType, const size_t threadId,
      const std::shared_ptr<search::ThreadController>& controller,
      const search::SearchType& searchType, const std::uint_fast32_t seed,
      const std::optional<std::chrono::milliseconds> timeLimit,
      std::shared_ptr<const bool>& shouldStop,
      const std::function<void(const search::SavedAssignment&,
                               search::ThreadController&, Int threadId)>&
          onSolution,
      const std::function<void(bool)>& onFinish)
      : _invariantGraph(invariantGraph),
        _outputVarNodeIds(std::move(outputVarNodeIds)),
        _annealingScheduleFactory(annealingScheduleFactory),
        _problemType(problemType),
        _threadId(threadId),
        _threadController(controller),
        _searchType(searchType),
        _seed(seed),
        _timelimit(timeLimit),
        _shouldStop(shouldStop),
        _onSolution(onSolution),
        _onFinish(onFinish) {}

  void solve(logging::Logger& logger);

  [[nodiscard]] std::unique_ptr<search::MetaHeuristic> createMetaHeuristic(
      logging::Logger&, search::RandomProvider&,
      const search::Assignment&) const;

  [[gnu::always_inline]] [[nodiscard]] std::vector<invariantgraph::VarNodeId>
  getOutputVarNodeIds() {
    return _outputVarNodeIds;
  }
};

}  // namespace atlantis