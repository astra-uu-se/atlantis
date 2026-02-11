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
class FznBackend;

class SolverThread {
  std::shared_ptr<const invariantgraph::FznInvariantGraph> _invariantGraph;
  std::vector<invariantgraph::VarNodeId> _outputVarNodeIds;
  std::shared_ptr<const search::AnnealingScheduleFactory>
      _annealingScheduleFactory;
  fznparser::ProblemType _problemType;
  size_t _threadId;
  std::shared_ptr<search::ThreadController> _threadController;
  search::SearchType _searchType;

  // Optional arguments
  std::uint_fast32_t _seed;
  std::optional<std::chrono::milliseconds> _timelimit;
  std::shared_ptr<const bool> _shouldStop;

 public:
  explicit SolverThread(FznBackend&, size_t threadId);
  explicit SolverThread(
      const std::shared_ptr<const invariantgraph::FznInvariantGraph>&
          invariantGraph,
      std::vector<invariantgraph::VarNodeId>&& outputVarNodeIds,
      fznparser::ProblemType problemType,
      const std::shared_ptr<const search::AnnealingScheduleFactory>&
          annealingFactorySchedule,
      size_t threadId,
      const std::shared_ptr<search::ThreadController>& controller,
      search::SearchType searchType,
      std::uint_fast32_t seed,
      std::optional<std::chrono::milliseconds> timeLimit,
      const std::shared_ptr<const bool>& shouldStop);

  void solve();

  [[nodiscard]] std::unique_ptr<search::MetaHeuristic> createMetaHeuristic(
      search::RandomProvider&, const search::Assignment&) const;

  [[gnu::always_inline]] [[nodiscard]] std::vector<invariantgraph::VarNodeId>
  getOutputVarNodeIds() {
    return _outputVarNodeIds;
  }
};

}  // namespace atlantis