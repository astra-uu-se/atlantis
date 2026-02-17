#pragma once

#include <filesystem>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "search/annealing/annealingScheduleFactory.hpp"
#include "search/savedAssignment.hpp"
#include "search/searchProcedure.hpp"
#include "search/threadController.hpp"

namespace atlantis {
class FznBackend;

class SolverThread {
  std::shared_ptr<const invariantgraph::FznInvariantGraph> _invariantGraph;
  std::vector<invariantgraph::VarNodeId> _outputVarNodeIds;
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
      size_t threadId,
      const std::shared_ptr<search::ThreadController>& controller,
      search::SearchType searchType,
      std::uint_fast32_t seed,
      std::optional<std::chrono::milliseconds> timeLimit,
      const std::shared_ptr<const bool>& shouldStop);

  void solve();

  [[gnu::always_inline]] [[nodiscard]] std::vector<invariantgraph::VarNodeId>
  getOutputVarNodeIds() {
    return _outputVarNodeIds;
  }
};

}  // namespace atlantis