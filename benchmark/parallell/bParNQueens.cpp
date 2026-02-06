#include <benchmark/benchmark.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "atlantis/fznBackend.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class ParNQueens : public ::benchmark::Fixture {
 public:
  static std::vector<std::string> instances;

  std::shared_ptr<FznBackend> backend{nullptr};

  long instance{-1};
  size_t numThreads{0};
  search::SearchType searchType{search::SearchType::BESTCOST};

  std::vector<std::chrono::milliseconds> timelimits;

  logging::Logger logger{stdout, logging::Level::LVL_ERROR};

  static void populateInstances() {
    instances = createInstances(std::string(FZN_DIR) + "/n_queens");
  }

  static size_t size() { return instances.size(); }

  void SetUp(const ::benchmark::State& state) override {
    instance = state.range(0);
    timelimits = defaultTimelimits();
    numThreads = state.range(1);
    searchType = intToSearchType(state.range(2));

    assert(0 <= instance && instance < static_cast<long>(instances.size()));

    std::filesystem::path modelFilePath(instances.at(instance).c_str());
    backend = std::make_shared<FznBackend>(logger, std::move(modelFilePath),
                                           numThreads, searchType);
  }

  void TearDown(const ::benchmark::State&) override { backend = nullptr; }
};

std::vector<std::string> ParNQueens::instances;

BENCHMARK_DEFINE_F(ParNQueens, run)(::benchmark::State& st) {
  st.SetLabel(instances.at(instance));
  std::vector<size_t> solved(timelimits.size(), 0);
#ifdef MORE_STATS
  std::vector sharedImprovingSolutions(timelimits.size(),
                                       std::vector<size_t>(numThreads, 0));
  std::vector metaStatAttempted(timelimits.size(),
                                std::vector<size_t>(numThreads, 0));
  std::vector metaStatAccepted(timelimits.size(),
                               std::vector<size_t>(numThreads, 0));
  std::vector metaStatUphillAttempted(timelimits.size(),
                                      std::vector<size_t>(numThreads, 0));
  std::vector metaStatUphillAccepted(timelimits.size(),
                                     std::vector<size_t>(numThreads, 0));
  std::vector metaStatImproving(timelimits.size(),
                                std::vector<size_t>(numThreads, 0));
  std::vector metaStatRounds(timelimits.size(),
                             std::vector<size_t>(numThreads, 0));
#endif
  backend->setOnFinish([](bool) {});
  backend->setTimelimit(timelimits.back());

  std::vector<std::chrono::time_point<std::chrono::steady_clock>> deadlines;
  deadlines.reserve(timelimits.size());
  for (const auto& tl : timelimits) {
    deadlines.emplace_back(std::chrono::steady_clock::now() + tl);
  }
  backend->setOnSolution(
      [&](const search::SavedAssignment&,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
        for (size_t i = 0; i < timelimits.size(); i++) {
          if (deadlines[i] < std::chrono::steady_clock::now()) {
            continue;
          }
          solved[i] = 1;
        }
      });

#ifdef MORE_STATS
  std::vector<std::shared_ptr<search::SearchStatistics>> threadStatistics;
  threadStatistics.reserve(numThreads);
  backend->setOnMove([&](const search::ThreadController& controller) {
    auto threadStatsOptional = controller.getStats();
    if (!threadStatsOptional.has_value()) return;
    threadStatistics = threadStatsOptional.value();

    for (size_t i = 0; i < timelimits.size(); i++) {
      if (deadlines[i] < std::chrono::steady_clock::now()) {
        continue;
      }
      for (size_t t = 0; t < numThreads; t++) {
        sharedImprovingSolutions[i][t] =
            stoi(threadStatistics[t]->getValue("improvingSolutions"));
        std::optional<std::shared_ptr<search::RoundStatistics>> metaStats =
            threadStatistics[t]->getRoundStatistics();
        if (metaStats.has_value()) {
          metaStatAttempted[i][t] = metaStats.value()->attemptedMoves;
          metaStatAccepted[i][t] = metaStats.value()->acceptedMoves;
          metaStatUphillAttempted[i][t] =
              metaStats.value()->uphillAttemptedMoves;
          metaStatUphillAccepted[i][t] = metaStats.value()->uphillAcceptedMoves;
          metaStatImproving[i][t] = metaStats.value()->improvingMoves;
          metaStatRounds[i][t] = metaStats.value()->rounds;
        }
      }
    }
  });
#endif

  for ([[maybe_unused]] const auto& _ : st) {
    backend->solve(logger);

    backend->join(logger);
  }
  for (size_t i = 0; i < timelimits.size(); i++) {
    const std::string prefix = std::to_string(timelimits[i].count());
    st.counters[prefix + "/solved"] = static_cast<double>(solved[i]);
#ifdef MORE_STATS
    for (size_t t = 0; t < numThreads; t++) {
      st.counters[prefix + "/thread" + std::to_string(t) +
                  "/improvedSolutionsFound"] = sharedImprovingSolutions[i][t];

      st.counters[prefix + "/thread" + std::to_string(t) + "/attemptedMoves"] =
          metaStatAttempted[i][t];
      st.counters[prefix + "/thread" + std::to_string(t) + "/acceptedMoves"] =
          metaStatAccepted[i][t];
      st.counters[prefix + "/thread" + std::to_string(t) +
                  "/uphillAttemptedMoves"] = metaStatUphillAttempted[i][t];
      st.counters[prefix + "/thread" + std::to_string(t) +
                  "/uphillAcceptedMoves"] = metaStatUphillAccepted[i][t];
      st.counters[prefix + "/thread" + std::to_string(t) + "/improvingMoves"] =
          metaStatImproving[i][t];
      st.counters[prefix + "/thread" + std::to_string(t) + "/annealerRounds"] =
          metaStatRounds[i][t];
    }
#endif
  }
}

BENCHMARK_REGISTER_F(ParNQueens, run)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments<ParNQueens>)
    ->Iterations(1);

}  // namespace atlantis::benchmark
