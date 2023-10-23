#include <benchmark/benchmark.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::benchmark {

class ExtremeStatic : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  std::vector<propagation::VarId> staticInputVars;
  propagation::VarId objective;

  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<Int> staticVarValueDist;
  std::uniform_int_distribution<Int> staticVarIndexDist;
  size_t numInputs;
  Int lb, ub;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<propagation::PropagationEngine>();

    lb = 0;
    ub = 16;

    numInputs = state.range(0);

    engine->open();
    setEngineModes(*engine, state.range(1));

    for (size_t i = 0; i < numInputs; ++i) {
      staticInputVars.emplace_back(engine->makeIntVar(lb, lb, ub));
    }

    objective = engine->makeIntVar(lb * static_cast<Int>(numInputs),
                                   lb * static_cast<Int>(numInputs),
                                   ub * static_cast<Int>(numInputs));
    engine->makeInvariant<propagation::Linear>(*engine, objective,
                                               staticInputVars);

    engine->close();
    gen = std::mt19937(rd());
    staticVarIndexDist =
        std::uniform_int_distribution<Int>{0, static_cast<Int>(numInputs) - 1};
    staticVarValueDist = std::uniform_int_distribution<Int>{lb, ub};
  }

  void TearDown(const ::benchmark::State&) { staticInputVars.clear(); }
};

// probe_single_non_index_var
// probe_single_index_var
// probe_single_move_index_input

BENCHMARK_DEFINE_F(ExtremeStatic, probe_single_var)
(::benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    engine->beginMove();
    engine->setValue(staticInputVars.at(staticVarIndexDist(gen)),
                     staticVarValueDist(gen));
    engine->endMove();

    engine->beginProbe();
    engine->query(objective);
    engine->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

//*

BENCHMARK_REGISTER_F(ExtremeStatic, probe_single_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/
}  // namespace atlantis::benchmark