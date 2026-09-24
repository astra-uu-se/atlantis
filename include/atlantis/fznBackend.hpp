#pragma once

#include <functional>
#include <fznparser/model.hpp>
#include <optional>
#include <thread>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchProcedure.hpp"
#include "atlantis/search/threadController.hpp"
#include "atlantis/solverThread.hpp"
#include "atlantis/types.hpp"
#include "atlantis/utils/fznOutput.hpp"

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
  enum class SolveOutcome {
    SATISFIABLE,
    UNSATISFIABLE,
    UNKNOWN,
  };

  static void onFinishDefault(SolveOutcome outcome);

  void onSolutionDefault(
      const search::SavedAssignment&,
      const std::optional<
          std::vector<std::shared_ptr<search::SearchStatistics>>>&) const;

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

  std::function<void(const search::SavedAssignment&,
                     const std::optional<std::vector<
                         std::shared_ptr<search::SearchStatistics>>>&)>
      _onSolution;
  std::function<void(SolveOutcome)> _onFinish = onFinishDefault;
  std::vector<std::thread> _threads{};
  std::shared_ptr<search::ThreadController> _threadController{nullptr};

  void handleSolverNotifications(
      const std::shared_ptr<search::ThreadController>& threadController) const;

 public:
  explicit FznBackend(
      fznparser::Model&& model, std::uint_fast32_t threadCount,
      search::SearchType searchType = search::SearchType::BEAMSEARCH);

  FznBackend(logging::Logger& logger, std::filesystem::path&& modelFile,
             std::uint_fast32_t threadCount = 1,
             search::SearchType searchType = search::SearchType::BEAMSEARCH);

  void solve(logging::Logger&);
  void join(logging::Logger&);

  void setTimelimit(std::optional<std::chrono::milliseconds> timeLimit);

  void setShouldStop(const std::shared_ptr<const bool>& shouldStop);

  [[nodiscard]] std::shared_ptr<const search::AnnealingScheduleFactory>
  annealingScheduleFactory() const;

  [[nodiscard]] std::shared_ptr<const invariantgraph::FznInvariantGraph>
  invariantGraph() const;

  [[nodiscard]] std::vector<invariantgraph::VarNodeId> outputVarNodeIds() const;

  [[nodiscard]] fznparser::ProblemType problemType() const;

  [[nodiscard]] std::shared_ptr<search::ThreadController> threadController();

  [[nodiscard]] search::SearchType searchType() const;

  [[nodiscard]] std::uint_fast32_t seed() const;

  [[nodiscard]] std::optional<std::chrono::milliseconds> timelimit() const;

  [[nodiscard]] std::shared_ptr<const bool> shouldStop() const;

  [[nodiscard]] std::function<
      void(const search::SavedAssignment&,
           const std::optional<
               std::vector<std::shared_ptr<search::SearchStatistics>>>&)>
  onSolution() const;

  [[nodiscard]] const std::function<void(SolveOutcome)>& onFinish() const;

  void setAnnealingScheduleFactory(
      const std::shared_ptr<search::AnnealingScheduleFactory>& factory);

  void setRandomSeed(std::uint_fast32_t seed);

  void setOnSolution(
      const std::function<void(const search::SavedAssignment&,
                               const std::optional<std::vector<std::shared_ptr<
                                   search::SearchStatistics>>>&)>& onSolution);

  void setDotFilePath(std::filesystem::path&& path);

  void setOnFinish(const std::function<void(SolveOutcome)>& onFinish);
};

}  // namespace atlantis
