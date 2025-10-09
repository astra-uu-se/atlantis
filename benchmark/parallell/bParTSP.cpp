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
  const std::string modelPath{std::string(FZN_DIR) + "/tsp_201.fzn"};
  std::shared_ptr<FznBackend> backend{nullptr};

  std::chrono::milliseconds timelimit{0};
  Int numThreads{0};
  search::SearchType searchType{search::SearchType::BESTCOST};

  logging::Logger logger{stdout, logging::Level::LVL_ERROR};

  void SetUp(const ::benchmark::State& state) override {
    timelimit = std::chrono::milliseconds(state.range(0));
    numThreads = state.range(1);
    searchType = intToSearchType(state.range(2));

    std::filesystem::path modelFilePath(modelPath.c_str());
    backend = std::make_shared<FznBackend>(logger, std::move(modelFilePath),
                                           numThreads, searchType);
  }

  void TearDown(const ::benchmark::State&) override { backend = nullptr; }
};

BENCHMARK_DEFINE_F(ParTSP, run)(::benchmark::State& st) {
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
    ->Apply(defaultArguments)
    ->Iterations(1);

}  // namespace atlantis::benchmark
