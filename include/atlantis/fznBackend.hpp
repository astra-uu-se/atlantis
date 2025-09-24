#pragma once

#include <functional>
#include <fznparser/model.hpp>
#include <optional>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "search/savedAssignment.hpp"
#include "search/searchProcedure.hpp"
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
  static void displaySolution(
      const invariantgraph::FznInvariantGraph& invariantGraph,
      const search::Assignment& assignment);
  static search::SavedAssignment onSolutionDefault(
      const invariantgraph::FznInvariantGraph&, const search::Assignment&,
      search::ThreadController& controller, Int threadId);
  static void onFinishDefault(bool);

 private:
  std::shared_ptr<fznparser::Model> _model;
  search::AnnealingScheduleFactory _annealingScheduleFactory;
  std::optional<std::chrono::milliseconds> _timelimit;
  std::uint_fast32_t _seed;
  std::optional<std::filesystem::path> _dotFilePath{};
  const std::uint_fast32_t _threadCount;
  search::SearchType _searchType;

  std::function<search::SavedAssignment(
      const invariantgraph::FznInvariantGraph& invariantGraph,
      const search::Assignment& assignment,
      search::ThreadController& controller, Int threadId)>
      _onSolution = onSolutionDefault;
  std::function<void(bool)> _onFinish = onFinishDefault;

 public:
  explicit FznBackend(fznparser::Model&& model,
                      const std::uint_fast32_t threadCount,
                      const search::SearchType searchType)
      : _model(std::make_shared<fznparser::Model>(std::move(model))),
        _seed(std::time(nullptr)),
        _threadCount(threadCount),
        _searchType(searchType) {}

  FznBackend(logging::Logger& logger, std::filesystem::path&& modelFile,
             std::uint_fast32_t threadCount = 1,
             search::SearchType searchType = search::SearchType::BEAMSEARCH);

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
          const invariantgraph::FznInvariantGraph&, const search::Assignment&,
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
