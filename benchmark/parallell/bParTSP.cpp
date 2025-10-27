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
  std::chrono::milliseconds timelimit{0};
  long numThreads{0};
  search::SearchType searchType{search::SearchType::BESTCOST};

  logging::Logger logger{stdout, logging::Level::LVL_ERROR};

  static void populateInstances() {
    instances = createInstances(std::string(FZN_DIR) + "/tsp");
  }

  static size_t size() {
    return instances.size();
  }

  void SetUp(const ::benchmark::State& state) override {
    instance = state.range(0);
    timelimit = std::chrono::milliseconds(state.range(1));
    numThreads = state.range(2);
    searchType = intToSearchType(state.range(3));

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
  size_t numSolutions{0};
  Int bestObjective{0};
  double totalObjective{0.0};
  backend->setOnSolution([&numSolutions, &bestObjective, &totalObjective](
                             const search::SavedAssignment& solution) {
    ++numSolutions;
    bestObjective = solution.getCost().getObjective();
    totalObjective += static_cast<double>(bestObjective);
  });
  backend->setOnFinish([](bool) {});
  backend->setTimelimit(timelimit);
  for ([[maybe_unused]] const auto& _ : st) {
    backend->solve(logger);
    backend->join(logger);
  }
  st.counters["solutions"] = static_cast<double>(numSolutions);
  st.counters["solutions_per_second"] = ::benchmark::Counter(
      static_cast<double>(numSolutions), ::benchmark::Counter::kIsRate);
  st.counters["objective_best"] = static_cast<double>(bestObjective);
  st.counters["objective_average"] =
      totalObjective / static_cast<double>(numSolutions);
}

BENCHMARK_REGISTER_F(ParTSP, run)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments<ParTSP>)
    ->Iterations(1);

}  // namespace atlantis::benchmark
