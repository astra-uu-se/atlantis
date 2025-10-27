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
  std::chrono::milliseconds timelimit{0};
  long numThreads{0};
  search::SearchType searchType{search::SearchType::BESTCOST};

  logging::Logger logger{stdout, logging::Level::LVL_ERROR};

  static void populateInstances() {
    instances = createInstances(std::string(FZN_DIR) + "/n_queens");
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

std::vector<std::string> ParNQueens::instances;

BENCHMARK_DEFINE_F(ParNQueens, run)(::benchmark::State& st) {
  st.SetLabel(instances.at(instance));
  size_t solved{0};
  double totalObjective{0.0};
  backend->setOnSolution([&solved](
                             const search::SavedAssignment& solution) {
    solved = 1;
  });
  backend->setOnFinish([](bool) {});
  backend->setTimelimit(timelimit);
  for ([[maybe_unused]] const auto& _ : st) {
    backend->solve(logger);
    backend->join(logger);
  }
  st.counters["solved"] = static_cast<double>(solved);
}

BENCHMARK_REGISTER_F(ParNQueens, run)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments<ParNQueens>)
    ->Iterations(1);

}  // namespace atlantis::benchmark
