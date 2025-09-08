#pragma once

#include <functional>
#include <fznparser/model.hpp>
#include <optional>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
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
  static void displaySolution(
      const invariantgraph::FznInvariantGraph& invariantGraph,
      const search::Assignment& assignment);
  static void onSolutionDefault(
      const invariantgraph::FznInvariantGraph&, const search::Assignment&,
      std::unordered_map<std::string_view, std::string>);
  static void onFinishDefault(bool);

 private:
  fznparser::Model _model;
  search::AnnealingScheduleFactory _annealingScheduleFactory;
  std::optional<std::chrono::milliseconds> _timelimit;
  std::uint_fast32_t _seed;
  std::optional<std::filesystem::path> _dotFilePath{};
  const std::uint_fast32_t _threadCount;

  std::function<void(const invariantgraph::FznInvariantGraph&,
                     const search::Assignment&,
                     std::unordered_map<std::string_view, std::string>)>
      _onSolution = onSolutionDefault;
  std::function<void(bool)> _onFinish = onFinishDefault;

 public:
  explicit FznBackend(fznparser::Model&& model,
                      const std::uint_fast32_t threadCount)
      : _model(std::move(model)),
        _seed(std::time(nullptr)),
        _threadCount(threadCount) {}

  FznBackend(logging::Logger& logger, std::filesystem::path&& modelFile,
             std::uint_fast32_t threadCount = 1);

  search::SearchStatistics solve(logging::Logger& logger);

  std::pair<search::SearchStatistics, search::Assignment> solveThread(
      logging::Logger& logger, uint_fast32_t threadId,
      ObjectiveDirection objective_direction,
      fznparser::ProblemType problemType,
      std::shared_ptr<search::AnnealingSchedule> schedule);

  void setTimelimit(std::optional<std::chrono::milliseconds> timeLimit) {
    _timelimit = timeLimit;
  }

  void setAnnealingScheduleFactory(search::AnnealingScheduleFactory&& factory) {
    _annealingScheduleFactory = factory;
  }

  void setRandomSeed(std::uint_fast32_t seed) { _seed = seed; }

  void setOnSolution(
      const std::function<void(
          const invariantgraph::FznInvariantGraph&, const search::Assignment&,
          std::unordered_map<std::string_view, std::string>)>& onSolution) {
    _onSolution = onSolution;
  }

  void setDotFilePath(std::filesystem::path&& path) {
    _dotFilePath = std::optional<std::filesystem::path>(std::move(path));
  }

  void setOnFinish(const std::function<void(bool)>& onFinish) {
    _onFinish = onFinish;
  }
};

}  // namespace atlantis
