#include <benchmark/benchmark.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "misc/logging.hpp"
#include "propagation/invariants/elementVar.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::benchmark {

class ExtremeDynamic : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  propagation::VarId staticInputVar;
  std::vector<propagation::VarId> dynamicInputVars;
  std::vector<propagation::VarId> outputVars;
  propagation::VarId objective;

  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<Int> staticVarValueDist;
  std::uniform_int_distribution<Int> dynamicVarValueDist;
  size_t numInvariants;
  int lb, ub;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<propagation::PropagationEngine>();

    lb = 0;
    ub = 1000;

    numInvariants = state.range(0);

    engine->open();
    setEngineModes(*engine, state.range(1));

    staticInputVar = engine->makeIntVar(0, 0, static_cast<Int>(numInvariants));
    for (size_t i = 0; i < numInvariants; ++i) {
      dynamicInputVars.emplace_back(engine->makeIntVar(lb, lb, ub));
      outputVars.emplace_back(engine->makeIntVar(lb, lb, ub));
    }

    for (size_t i = 0; i < numInvariants; ++i) {
      engine->makeInvariant<propagation::ElementVar>(
          *engine, outputVars.at(i), staticInputVar, dynamicInputVars, 0);
    }

    objective = engine->makeIntVar(lb * static_cast<Int>(numInvariants),
                                   lb * static_cast<Int>(numInvariants),
                                   ub * static_cast<Int>(numInvariants));
    engine->makeInvariant<propagation::ElementVar>(
        *engine, objective, staticInputVar, outputVars, 0);

    engine->close();
    gen = std::mt19937(rd());
    staticVarValueDist = std::uniform_int_distribution<Int>{
        0, static_cast<Int>(numInvariants) - 1};
    dynamicVarValueDist = std::uniform_int_distribution<Int>{lb, ub};
  }

  void TearDown(const ::benchmark::State&) {
    dynamicInputVars.clear();
    outputVars.clear();
  }
};

// probe_single_non_index_var
// probe_single_index_var
// probe_single_move_index_input

BENCHMARK_DEFINE_F(ExtremeDynamic, probe_static_var)
(::benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    // Perform move
    engine->beginMove();
    engine->setValue(staticInputVar, staticVarValueDist(gen));
    engine->endMove();

    engine->beginProbe();
    engine->query(objective);
    engine->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(ExtremeDynamic, probe_single_dynamic_var)
(::benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    engine->beginMove();
    engine->setValue(dynamicInputVars.at(staticVarValueDist(gen)),
                     dynamicVarValueDist(gen));
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

BENCHMARK_REGISTER_F(ExtremeDynamic, probe_static_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

BENCHMARK_REGISTER_F(ExtremeDynamic, probe_single_dynamic_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/
}  // namespace atlantis::benchmark