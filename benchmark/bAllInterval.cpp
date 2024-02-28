#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/absDiff.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class AllInterval : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::vector<propagation::VarId> inputVars;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  size_t n{0};

  propagation::VarId totalViolation = propagation::NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_unique<propagation::Solver>();
    if (state.range(0) < 0) {
      throw std::runtime_error("n must be non-negative.");
    }
    n = state.range(0);
    inputVars.resize(n, propagation::NULL_ID);
    std::vector<propagation::VarId> violationVars(n - 1, propagation::NULL_ID);
    // number of constraints: n
    // total number of static input variables: 3 * n
    solver->open();

    setSolverMode(*solver, static_cast<int>(state.range(1)));

    for (size_t i = 0; i < n; ++i) {
      assert(i < inputVars.size());
      inputVars[i] =
          solver->makeIntVar(static_cast<Int>(i), 0, static_cast<Int>(n) - 1);
    }
    // Creating n - 1 invariants, each having two inputs and one output
    for (size_t i = 0; i < n - 1; ++i) {
      assert(i < violationVars.size());
      violationVars[i] =
          solver->makeIntVar(static_cast<Int>(i), 0, static_cast<Int>(n) - 1);
      assert(i + 1 < inputVars.size());
      solver->makeInvariant<propagation::AbsDiff>(
          *solver, violationVars[i], inputVars[i], inputVars[i + 1]);
    }

    totalViolation = solver->makeIntVar(0, 0, static_cast<Int>(n));
    // Creating one invariant, taking n input variables and one output
    solver->makeViolationInvariant<propagation::AllDifferent>(
        *solver, totalViolation,
        std::vector<propagation::VarId>(violationVars));

    solver->close();

    gen = std::mt19937(rd());

    distribution =
        std::uniform_int_distribution<Int>{0, static_cast<Int>(n) - 1};
  }

  void TearDown(const ::benchmark::State&) override { inputVars.clear(); }
};

BENCHMARK_DEFINE_F(AllInterval, probe_single_swap)(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    const size_t i = distribution(gen);
    assert(i < inputVars.size());
    const size_t j = distribution(gen);
    assert(j < inputVars.size());
    const Int oldI = solver->committedValue(inputVars[i]);
    const Int oldJ = solver->committedValue(inputVars[j]);
    // Perform random swap
    solver->beginMove();
    solver->setValue(inputVars[i], oldJ);
    solver->setValue(inputVars[j], oldI);
    solver->endMove();

    solver->beginProbe();
    solver->query(totalViolation);
    solver->endProbe();
    ++probes;
    assert(all_in_range(0, n - 1, [&](const size_t a) {
      return all_in_range(a + 1, n, [&](const size_t b) {
        return solver->committedValue(inputVars.at(a)) !=
                   solver->committedValue(inputVars.at(b)) &&
               solver->currentValue(inputVars.at(a)) !=
                   solver->currentValue(inputVars.at(b));
      });
    }));
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(AllInterval, probe_all_swap)(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
        const Int oldI = solver->committedValue(inputVars[i]);
        const Int oldJ = solver->committedValue(inputVars[j]);
        solver->beginMove();
        solver->setValue(inputVars[i], oldJ);
        solver->setValue(inputVars[j], oldI);
        solver->endMove();

        solver->beginProbe();
        solver->query(totalViolation);
        solver->endProbe();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(AllInterval, commit_single_swap)(::benchmark::State& st) {
  int commits = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    const size_t i = distribution(gen);
    assert(i < inputVars.size());
    const size_t j = distribution(gen);
    assert(j < inputVars.size());
    const Int oldI = solver->committedValue(inputVars[i]);
    const Int oldJ = solver->committedValue(inputVars[j]);
    // Perform random swap
    solver->beginMove();
    solver->setValue(inputVars[i], oldJ);
    solver->setValue(inputVars[j], oldI);
    solver->endMove();

    solver->beginCommit();
    solver->query(totalViolation);
    solver->endCommit();

    ++commits;
  }

  st.counters["seconds_per_commit"] = ::benchmark::Counter(
      commits, ::benchmark::Counter::kIsRate | ::benchmark::Counter::kInvert);
}

//*
BENCHMARK_REGISTER_F(AllInterval, probe_single_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/
/*
BENCHMARK_REGISTER_F(AllInterval, probe_all_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/
/*

static void commitArguments(::benchmark::internal::Benchmark* benchmark) {
  for (int n = 10; n <= 10; n += 10) {
    benchmark->Args({n, 0});
  }
#ifndef NDEBUG
    break;
#endif
}
BENCHMARK_REGISTER_F(AllInterval, commit_single_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(commitArguments);
//*/
}  // namespace atlantis::benchmark
