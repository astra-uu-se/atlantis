#pragma once

#include <filesystem>
#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/objective.hpp"
#include "search/annealing/annealingSchedule.hpp"
#include "search/savedAssignment.hpp"
#include "search/threadController.hpp"
#include "types.hpp"

namespace atlantis {

class SolverThread {
  std::shared_ptr<const invariantgraph::FznInvariantGraph>& _invariantGraph;
  fznparser::ProblemType _problemType;
  std::shared_ptr<search::AnnealingSchedule> _schedule;
  size_t _threadId;
  std::shared_ptr<search::ThreadController> _controller;

  // Optional arguments
  std::uint_fast32_t _seed;
  std::optional<std::chrono::milliseconds> _timelimit;

  std::function<search::SavedAssignment(
      const search::Assignment&,
      const FznOutput&,
      search::ThreadController&, Int threadId)>
      _onSolution;

  std::function<void(bool)> _onFinish;

 public:
  explicit SolverThread(
      std::shared_ptr<const invariantgraph::FznInvariantGraph>&& invariantGraph,
      const fznparser::ProblemType problemType,
      const std::shared_ptr<search::AnnealingSchedule>& schedule,
      const size_t threadId,
      const std::shared_ptr<search::ThreadController>& controller,
      const std::uint_fast32_t seed,
      const std::optional<std::chrono::milliseconds> timeLimit,
      const std::function<search::SavedAssignment(
          const search::Assignment&,
          const FznOutput&,
          search::ThreadController&, Int threadId)>& onSolution,
      const std::function<void(bool)>& onFinish)
      : _invariantGraph(invariantGraph),
        _problemType(problemType),
        _schedule(schedule),
        _threadId(threadId),
        _controller(controller),
        _seed(seed),
        _timelimit(timeLimit),
        _onSolution(onSolution),
        _onFinish(onFinish) {}

  void solve(logging::Logger& logger);
};

}  // namespace atlantis