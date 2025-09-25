#pragma once

#include <functional>
#include <fznparser/model.hpp>
#include <optional>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/solverMapping.hpp"
#include "search/savedAssignment.hpp"
#include "search/threadController.hpp"
#include "types.hpp"

namespace atlantis {

namespace invariantgraph {
class FznInvariantGraph;
}

namespace logging {
class Logger;
}

namespace search {
class Assignment;
class SearchStatistics;
}  // namespace search

class FznBackend {
 public:
  void displaySolution(const invariantgraph::SolverMapping&,
      const search::Assignment&) const;
  static void onFinishDefault(bool);
  search::SavedAssignment onSolutionDefault(const invariantgraph::SolverMapping&,
    const search::Assignment&, search::ThreadController&,
    const Int threadId) const;

 private:
  std::shared_ptr<invariantgraph::FznInvariantGraph> _invariantGraph;
  std::shared_ptr<fznparser::Model> _model;
  search::AnnealingScheduleFactory _annealingScheduleFactory;
  std::optional<std::chrono::milliseconds> _timelimit;
  std::uint_fast32_t _seed;
  std::optional<std::filesystem::path> _dotFilePath{};
  const std::uint_fast32_t _threadCount;

  std::function<search::SavedAssignment(
      const invariantgraph::SolverMapping&,
      const search::Assignment&,
      search::ThreadController&, Int threadId)>
      _onSolution;
  std::function<void(bool)> _onFinish = onFinishDefault;

 public:
  explicit FznBackend(fznparser::Model&& model,
                      const std::uint_fast32_t threadCount)
      : _invariantGraph(std::make_shared<invariantgraph::FznInvariantGraph>(true)),
        _model(std::make_shared<fznparser::Model>(std::move(model))),
        _seed(std::time(nullptr)),
        _threadCount(threadCount),
        _onSolution([this](const invariantgraph::SolverMapping& mapping, const search::Assignment& assignment, search::ThreadController& controller, Int threadId) {
          return onSolutionDefault(mapping, assignment, controller, threadId);
        }){}

  FznBackend(logging::Logger& logger, std::filesystem::path&& modelFile,
             std::uint_fast32_t threadCount = 1);

  void solve(logging::Logger& logger);

  void setTimelimit(std::optional<std::chrono::milliseconds> timeLimit) {
    _timelimit = timeLimit;
  }

  void setAnnealingScheduleFactory(search::AnnealingScheduleFactory&& factory) {
    _annealingScheduleFactory = factory;
  }

  void setRandomSeed(std::uint_fast32_t seed) { _seed = seed; }

  void setOnSolution(
      const std::function<search::SavedAssignment(
          const invariantgraph::SolverMapping&, const search::Assignment&,
          search::ThreadController&, Int)>& onSolution) {
    _onSolution = onSolution;
  }

  void setDotFilePath(std::filesystem::path&& path) {
    _dotFilePath = std::optional(std::move(path));
  }

  void setOnFinish(const std::function<void(bool)>& onFinish) {
    _onFinish = onFinish;
  }
};

}  // namespace atlantis
