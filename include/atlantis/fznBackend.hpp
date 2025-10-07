#pragma once

#include <functional>
#include <fznparser/model.hpp>
#include <optional>
#include <thread>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/solverMapping.hpp"
#include "search/savedAssignment.hpp"
#include "search/searchProcedure.hpp"
#include "search/threadController.hpp"
#include "solverThread.hpp"
#include "types.hpp"
#include "utils/fznOutput.hpp"

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
  void displaySolution(const search::SavedAssignment&) const;

  static void onFinishDefault(bool);

  void onSolutionDefault(const search::SavedAssignment&,
                         const search::ThreadController&, Int threadId) const;

 private:
  std::shared_ptr<invariantgraph::FznInvariantGraph> _invariantGraph;
  std::shared_ptr<fznparser::Model> _model;
  std::shared_ptr<search::AnnealingScheduleFactory> _annealingScheduleFactory;
  std::optional<std::chrono::milliseconds> _timelimit;
  std::uint_fast32_t _seed;
  std::optional<std::filesystem::path> _dotFilePath{};
  const std::uint_fast32_t _threadCount;
  search::SearchType _searchType;
  std::unique_ptr<FznOutput> _fznOutput{nullptr};
  std::shared_ptr<const bool> _shouldStop{nullptr};

  std::function<void(const search::SavedAssignment&, search::ThreadController&,
                     Int threadId)>
      _onSolution;
  std::function<void(bool)> _onFinish = onFinishDefault;
  std::vector<std::thread> _threads{};
  std::shared_ptr<search::ThreadController> _threadController{nullptr};

  void handleSolverIO(
      const std::shared_ptr<search::ThreadController>& threadController) const;

 public:
  explicit FznBackend(
      fznparser::Model&& model, const std::uint_fast32_t threadCount,
      search::SearchType searchType = search::SearchType::BEAMSEARCH);

  FznBackend(logging::Logger& logger, std::filesystem::path&& modelFile,
             std::uint_fast32_t threadCount = 1,
             search::SearchType searchType = search::SearchType::BEAMSEARCH);

  void solve(logging::Logger&);
  void join(logging::Logger&);

  void setTimelimit(std::optional<std::chrono::milliseconds> timeLimit) {
    _timelimit = timeLimit;
  }

  void setShouldStop(const std::shared_ptr<const bool>& shouldStop) {
    _shouldStop = shouldStop;
  }

  [[nodiscard]] std::shared_ptr<const search::AnnealingScheduleFactory>
  annealingScheduleFactory() const {
    return _annealingScheduleFactory;
  }

  [[nodiscard]] std::shared_ptr<const invariantgraph::FznInvariantGraph>
  invariantGraph() const {
    return _invariantGraph;
  }

  [[nodiscard]] std::vector<invariantgraph::VarNodeId> outputVarNodeIds()
      const {
    return _fznOutput->varNodeIds();
  }

  [[nodiscard]] fznparser::ProblemType problemType() const {
    return _model->solveType().problemType();
  }

  [[nodiscard]] std::shared_ptr<search::ThreadController> threadController() {
    return _threadController;
  }

  [[nodiscard]] search::SearchType searchType() const { return _searchType; }

  [[nodiscard]] std::uint_fast32_t seed() const { return _seed; }

  [[nodiscard]] std::optional<std::chrono::milliseconds> timelimit() const {
    return _timelimit;
  }

  [[nodiscard]] std::shared_ptr<const bool> shouldStop() const {
    return _shouldStop;
  }

  [[nodiscard]] const std::function<void(
      const search::SavedAssignment&, search::ThreadController&, Int threadId)>&
  onSolution() const {
    return _onSolution;
  }

  [[nodiscard]] const std::function<void(bool)>& onFinish() const {
    return _onFinish;
  };

  void setAnnealingScheduleFactory(
      const std::shared_ptr<search::AnnealingScheduleFactory>& factory) {
    _annealingScheduleFactory = factory;
  }

  void setRandomSeed(std::uint_fast32_t seed) { _seed = seed; }

  void setOnSolution(
      const std::function<void(const search::SavedAssignment&,
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
