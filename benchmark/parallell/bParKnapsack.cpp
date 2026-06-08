#include <benchmark/benchmark.h>

#include <boost/chrono/system_clocks.hpp>
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

class ParKnapsack : public ::benchmark::Fixture {
 public:
  static std::vector<std::string> instances;
  std::shared_ptr<FznBackend> backend{nullptr};

  long instance{-1};
  long numThreads{0};
  search::SearchType searchType{search::SearchType::BESTCOST};

  std::vector<std::chrono::milliseconds> timelimits;

  logging::Logger logger{stdout, logging::Level::LVL_ERROR};

  static void populateInstances() {
    instances = createInstances(std::string(FZN_DIR) + "/knapsack");
  }

  static size_t size() { return instances.size(); }

  void SetUp(const ::benchmark::State& state) override {
    instance = state.range(0);
    numThreads = state.range(1);
    searchType = intToSearchType(state.range(2));

    timelimits = defaultTimelimits();

    assert(0 <= instance && instance < static_cast<long>(instances.size()));

    std::filesystem::path modelFilePath(instances.at(instance).c_str());
    backend = std::make_shared<FznBackend>(logger, std::move(modelFilePath),
                                           numThreads, searchType);
  }

  void TearDown(const ::benchmark::State&) override { backend = nullptr; }
};

std::vector<std::string> ParKnapsack::instances;

BENCHMARK_DEFINE_F(ParKnapsack, run)(::benchmark::State& st) {
  st.SetLabel(instances.at(instance));
  std::vector<size_t> numSolutions(timelimits.size(), 0);
  std::vector<Int> bestObjective(timelimits.size(), 0);
  std::vector<Int> bestViolation(timelimits.size(), 0);
  std::vector<double> totalObjective(timelimits.size(), 0.0);
  backend->setOnFinish([](FznBackend::SolveOutcome) {});
  backend->setTimelimit(timelimits.back());

  std::vector<std::chrono::time_point<std::chrono::steady_clock>> deadlines;
  deadlines.reserve(timelimits.size());
  for (const auto& tl : timelimits) {
    deadlines.emplace_back(std::chrono::steady_clock::now() + tl);
  }

  backend->setOnSolution(
      [&](const search::SavedAssignment& solution,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
        assert(solution.cost().violation() >= 0);
        for (size_t i = 0; i < timelimits.size(); i++) {
          if (deadlines[i] < std::chrono::steady_clock::now()) {
            continue;
          }
          ++numSolutions[i];
          bestObjective[i] = solution.cost().objective();
          bestViolation[i] = solution.cost().violation();
          totalObjective[i] += static_cast<double>(bestObjective[i]);
        }
      });

  for ([[maybe_unused]] const auto& _ : st) {
    backend->solve(logger);
    backend->join(logger);
  }
  for (size_t i = 0; i < timelimits.size(); i++) {
    const std::string prefix = std::to_string(timelimits[i].count());
    st.counters[prefix + "/solutions"] = static_cast<double>(numSolutions[i]);
    st.counters[prefix + "/solutions_per_second"] = ::benchmark::Counter(
        static_cast<double>(numSolutions[i]), ::benchmark::Counter::kIsRate);
    st.counters[prefix + "/objective_best"] =
        static_cast<double>(bestObjective[i]);
    st.counters[prefix + "/violation_best"] =
        static_cast<double>(bestViolation[i]);
    st.counters[prefix + "/objective_average"] =
        totalObjective[i] / static_cast<double>(numSolutions[i]);
  }
}

BENCHMARK_REGISTER_F(ParKnapsack, run)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments<ParKnapsack>)
    ->Iterations(1);

}  // namespace atlantis::benchmark
