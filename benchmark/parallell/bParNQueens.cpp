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
  std::vector<size_t> bestViolation(timelimits.size(), 0);
  backend->setTimelimit(timelimits.back());

  std::vector<std::chrono::time_point<std::chrono::steady_clock>> deadlines;
  deadlines.reserve(timelimits.size());
  for (const auto& tl : timelimits) {
    deadlines.emplace_back(std::chrono::steady_clock::now() + tl);
  }
  backend->setOnFinish(
      [&](FznBackend::SolveOutcome) {
        const auto time = std::chrono::steady_clock::now();
        for (size_t i = 0; i < timelimits.size(); i++) {
          if (deadlines[i] < time) {
            continue;
          }
          solved[i] = 1;
        }
      });
  backend->setOnSolution(
      [&](const search::SavedAssignment& solution,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
                const auto time = std::chrono::steady_clock::now();
                for (size_t i = 0; i < timelimits.size(); i++) {
                  if (deadlines[i] < time) {
                    continue;
                  }
                  bestViolation[i] = solution.cost().violation();
                }
              });

  for ([[maybe_unused]] const auto& _ : st) {
    backend->solve(logger);
    backend->join(logger);
  }
  for (size_t i = 0; i < timelimits.size(); i++) {
    const std::string prefix = std::to_string(timelimits[i].count());
    st.counters[prefix + "/solved"] = static_cast<double>(solved[i]);
    st.counters[prefix + "/violation_best"] =
        static_cast<double>(bestViolation[i]);
  }
}

BENCHMARK_REGISTER_F(ParNQueens, run)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments<ParNQueens>)
    ->Iterations(1);

}  // namespace atlantis::benchmark
