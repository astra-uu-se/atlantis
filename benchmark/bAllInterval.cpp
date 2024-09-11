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
  std::shared_ptr<propagation::Solver> _solver;
  std::vector<propagation::VarViewId> inputVarIds;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  size_t n{0};

  propagation::VarViewId totalViolation = propagation::NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    _solver = std::make_shared<propagation::Solver>();
    if (state.range(0) < 0) {
      throw std::runtime_error("n must be non-negative.");
    }
    n = state.range(0);
    inputVarIds.resize(n, propagation::NULL_ID);
    std::vector<propagation::VarViewId> violationVars;
    violationVars.reserve(n - 1);
    // number of constraints: n
    // total number of static input variables: 3 * n
    _solver->open();

    setSolverMode(*_solver, static_cast<int>(state.range(1)));

    for (size_t i = 0; i < n; ++i) {
      inputVarIds.emplace_back(
          _solver->makeIntVar(static_cast<Int>(i), 0, static_cast<Int>(n) - 1));
    }
    // Creating n - 1 invariants, each having two inputs and one output
    for (size_t i = 0; i < n - 1; ++i) {
      violationVars.emplace_back(
          _solver->makeIntVar(static_cast<Int>(i), 0, static_cast<Int>(n) - 1));
      assert(i + 1 < inputVarIds.size());
      _solver->makeInvariant<propagation::AbsDiff>(
          *_solver, violationVars[i], inputVarIds[i], inputVarIds[i + 1]);
    }

    totalViolation = _solver->makeIntVar(0, 0, static_cast<Int>(n));
    // Creating one invariant, taking n input variables and one output
    _solver->makeViolationInvariant<propagation::AllDifferent>(
        *_solver, totalViolation,
        std::vector<propagation::VarViewId>(violationVars));

    _solver->close();

    gen = std::mt19937(rd());

    distribution =
        std::uniform_int_distribution<Int>{0, static_cast<Int>(n) - 1};
  }

  void TearDown(const ::benchmark::State&) override { inputVarIds.clear(); }
};

BENCHMARK_DEFINE_F(AllInterval, probe_single_swap)(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    const size_t i = distribution(gen);
    assert(i < inputVarIds.size());
    const size_t j = distribution(gen);
    assert(j < inputVarIds.size());
    const Int oldI = _solver->committedValue(inputVarIds[i]);
    const Int oldJ = _solver->committedValue(inputVarIds[j]);
    // Perform random swap
    _solver->beginMove();
    _solver->setValue(inputVarIds[i], oldJ);
    _solver->setValue(inputVarIds[j], oldI);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(totalViolation);
    _solver->endProbe();
    ++probes;
    assert(all_in_range(0, n - 1, [&](const size_t a) {
      return all_in_range(a + 1, n, [&](const size_t b) {
        return _solver->committedValue(inputVarIds.at(a)) !=
                   _solver->committedValue(inputVarIds.at(b)) &&
               _solver->currentValue(inputVarIds.at(a)) !=
                   _solver->currentValue(inputVarIds.at(b));
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
        const Int oldI = _solver->committedValue(inputVarIds[i]);
        const Int oldJ = _solver->committedValue(inputVarIds[j]);
        _solver->beginMove();
        _solver->setValue(inputVarIds[i], oldJ);
        _solver->setValue(inputVarIds[j], oldI);
        _solver->endMove();

        _solver->beginProbe();
        _solver->query(totalViolation);
        _solver->endProbe();

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
    assert(i < inputVarIds.size());
    const size_t j = distribution(gen);
    assert(j < inputVarIds.size());
    const Int oldI = _solver->committedValue(inputVarIds[i]);
    const Int oldJ = _solver->committedValue(inputVarIds[j]);
    // Perform random swap
    _solver->beginMove();
    _solver->setValue(inputVarIds[i], oldJ);
    _solver->setValue(inputVarIds[j], oldI);
    _solver->endMove();

    _solver->beginCommit();
    _solver->query(totalViolation);
    _solver->endCommit();

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
