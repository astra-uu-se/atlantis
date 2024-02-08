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
#include "propagation/solver.hpp"

namespace atlantis::benchmark {

class ExtremeStatic : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::vector<propagation::VarId> staticInputVars;
  propagation::VarId objective;

  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<Int> staticVarValueDist;
  std::uniform_int_distribution<Int> staticVarIndexDist;
  size_t numInputs;
  Int lb{0};
  Int ub{0};

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_unique<propagation::Solver>();

    lb = 0;
    ub = 16;

    numInputs = state.range(0);

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(1)));

    for (size_t i = 0; i < numInputs; ++i) {
      staticInputVars.emplace_back(solver->makeIntVar(lb, lb, ub));
    }

    objective = solver->makeIntVar(lb * static_cast<Int>(numInputs),
                                   lb * static_cast<Int>(numInputs),
                                   ub * static_cast<Int>(numInputs));
    solver->makeInvariant<propagation::Linear>(
        *solver, objective, std::vector<propagation::VarId>(staticInputVars));

    solver->close();
    gen = std::mt19937(rd());
    staticVarIndexDist =
        std::uniform_int_distribution<Int>{0, static_cast<Int>(numInputs) - 1};
    staticVarValueDist = std::uniform_int_distribution<Int>{lb, ub};
  }

  void TearDown(const ::benchmark::State&) override { staticInputVars.clear(); }
};

// probe_single_non_index_var
// probe_single_index_var
// probe_single_move_index_input

BENCHMARK_DEFINE_F(ExtremeStatic, probe_single_var)
(::benchmark::State& st) {
  size_t probes = 0;
  for (const auto& _ : st) {
    solver->beginMove();
    solver->setValue(staticInputVars.at(staticVarIndexDist(gen)),
                     staticVarValueDist(gen));
    solver->endMove();

    solver->beginProbe();
    solver->query(objective);
    solver->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

//*

BENCHMARK_REGISTER_F(ExtremeStatic, probe_single_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/
}  // namespace atlantis::benchmark