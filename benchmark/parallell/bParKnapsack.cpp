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
  search::BanditAlgorithm banditAlgorithm{search::BanditAlgorithm::ETC};
  std::filesystem::path annealingSchedulePath =
    "benchmark/parallell/banditTestSchedule2.json";

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
    banditAlgorithm = intToBanditAlgorithm(state.range(3));

    timelimits = defaultTimelimits();

    assert(0 <= instance && instance < static_cast<long>(instances.size()));

    std::filesystem::path modelFilePath(instances.at(instance).c_str());
    backend = std::make_shared<FznBackend>(logger, std::move(modelFilePath),
                                           numThreads, searchType, banditAlgorithm);

    backend->setAnnealingScheduleFactory(
      std::make_shared<search::AnnealingScheduleFactory>(annealingSchedulePath));
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

  const size_t numArms = backend->annealingScheduleFactory()->armCount();
  auto armRecordings = std::vector<std::vector<Int>>();
  auto armBestViolation = std::vector<std::vector<Int>>();
  auto armBestObjective = std::vector<std::vector<Int>>();
  for (size_t _ = 0; _ < numArms; _++) {
    armRecordings.push_back(std::vector<Int>(timelimits.size(), 0));
    armBestViolation.push_back(std::vector<Int>(timelimits.size(), 0));
    armBestObjective.push_back(std::vector<Int>(timelimits.size(), 0));
  }

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
        assert(solution.getCost().getViolation() >= 0);
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

  backend->setOnArmRecording(
    [&](const std::shared_ptr<search::ArmStats>& stats, const size_t arm) {

      for (size_t i = 0; i < timelimits.size(); i++) {
        if (deadlines[i] < std::chrono::steady_clock::now()) {
          continue;
        }
        armRecordings[arm][i] = stats->timesRecorded;
        armBestViolation[arm][i] = stats->bestCost.value().violation();
        armBestObjective[arm][i] = stats->bestCost.value().objective();
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
    for (size_t arm = 0; arm < numArms; arm++) {
      st.counters[prefix + "/armRecordings/" + std::to_string(arm)] =
        static_cast<double>(armRecordings[arm][i]);
      st.counters[prefix + "/armBestViolation/" + std::to_string(arm)] =
        static_cast<double>(armBestViolation[arm][i]);
      st.counters[prefix + "/armBestObjective/" + std::to_string(arm)] =
        static_cast<double>(armBestObjective[arm][i]);
    }
  }
}

BENCHMARK_REGISTER_F(ParKnapsack, run)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments<ParKnapsack>)
    ->Iterations(1);

}  // namespace atlantis::benchmark
