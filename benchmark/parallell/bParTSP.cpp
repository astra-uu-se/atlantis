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

class ParTSP : public ::benchmark::Fixture {
 public:
  static std::vector<std::string> instances;

  std::shared_ptr<FznBackend> backend{nullptr};

  long instance{-1};
  size_t numThreads{0};
  search::SearchType searchType{search::SearchType::BESTCOST};

  std::vector<std::chrono::milliseconds> timelimits;

  logging::Logger logger{stdout, logging::Level::LVL_ERROR};

  static void populateInstances() {
    instances = createInstances(std::string(FZN_DIR) + "/tsp");
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

std::vector<std::string> ParTSP::instances;

BENCHMARK_DEFINE_F(ParTSP, run)(::benchmark::State& st) {
  st.SetLabel(instances.at(instance));
  std::vector<size_t> numSolutions(timelimits.size(), 0);
  std::vector<size_t> bestObjective(timelimits.size(), 0);
  std::vector<double> totalObjective(timelimits.size(), 0.0);
  std::vector threadNumProbes(timelimits.size(),
                              std::vector<size_t>(numThreads, 0));
  std::vector threadNumMoves(timelimits.size(),
                             std::vector<size_t>(numThreads, 0));
  std::vector threadNumSolutions(timelimits.size(),
                                 std::vector<size_t>(numThreads, 0));
  backend->setOnFinish([](bool) {});
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
        for (size_t i = 0; i < timelimits.size(); i++) {
          if (deadlines[i] < std::chrono::steady_clock::now()) {
            continue;
          }
          ++numSolutions[i];
          bestObjective[i] = solution.getCost().getObjective();
          totalObjective[i] += static_cast<double>(bestObjective[i]);
        }
      });

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
        threadNumProbes[i][t] = stoi(threadStatistics[t]->getValue("probes"));
        threadNumMoves[i][t] = stoi(threadStatistics[t]->getValue("moves"));
        threadNumSolutions[i][t] =
            stoi(threadStatistics[t]->getValue("improvingSolutions"));
      }
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
    st.counters[prefix + "/objective_average"] =
        totalObjective[i] / static_cast<double>(numSolutions[i]);
    for (size_t t = 0; t < numThreads; t++) {
      st.counters[prefix + "/thread" + std::to_string(t) + "/probes"] =
          threadNumProbes[i][t];
      st.counters[prefix + "/thread" + std::to_string(t) + "/moves"] =
          threadNumMoves[i][t];
      st.counters[prefix + "/thread" + std::to_string(t) + "/solutions"] =
          threadNumSolutions[i][t];
    }
  }
}

BENCHMARK_REGISTER_F(ParTSP, run)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments<ParTSP>)
    ->Iterations(1);

}  // namespace atlantis::benchmark
