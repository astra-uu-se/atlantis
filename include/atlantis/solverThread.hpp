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
  ObjectiveDirection _objectiveDirection;
  fznparser::ProblemType _problemType;
  std::shared_ptr<search::AnnealingSchedule> _schedule;
  Int _threadId;
  std::shared_ptr<search::ThreadController> _controller;

  // Necessary objects
  std::shared_ptr<fznparser::Model> _model;

  // Optional arguments
  std::uint_fast32_t _seed;
  std::optional<std::filesystem::path> _dotFilePath{};
  std::optional<std::chrono::milliseconds> _timelimit;

  std::function<search::SavedAssignment(
      const invariantgraph::FznInvariantGraph& invariantGraph,
      const search::Assignment& assignment,
      search::ThreadController& controller, Int threadId)>
      _onSolution;

  std::function<void(bool)> _onFinish;

 public:
  explicit SolverThread(
      const ObjectiveDirection objectiveDirection,
      const fznparser::ProblemType problemType,
      const std::shared_ptr<search::AnnealingSchedule>& schedule,
      const Int threadId,
      const std::shared_ptr<search::ThreadController>& controller,
      const std::shared_ptr<fznparser::Model>& model,
      const std::uint_fast32_t seed,
      std::optional<std::filesystem::path> dotFilePath,
      const std::optional<std::chrono::milliseconds> timeLimit,
      const std::function<search::SavedAssignment(
          const invariantgraph::FznInvariantGraph& invariantGraph,
          const search::Assignment& assignment,
          search::ThreadController& controller, Int threadId)>& onSolution,
      const std::function<void(bool)>& onFinish)
      : _objectiveDirection(objectiveDirection),
        _problemType(problemType),
        _schedule(schedule),
        _threadId(threadId),
        _controller(controller),
        _model(std::move(model)),
        _seed(seed),
        _dotFilePath(std::move(dotFilePath)),
        _timelimit(timeLimit),
        _onSolution(onSolution),
        _onFinish(onFinish) {}

  void saveInvariantGraph(
      const invariantgraph::FznInvariantGraph& invariantGraph) const;
  void solve(logging::Logger& logger);
};

}  // namespace atlantis