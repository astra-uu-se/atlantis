#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "propagation/constraints/allDifferent.hpp"
#include "propagation/invariants/absDiff.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::benchmark {

class AllInterval : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  std::vector<propagation::VarId> inputVars;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  size_t n;

  propagation::VarId totalViolation = propagation::NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    engine = std::make_unique<propagation::PropagationEngine>();
    if (state.range(0) < 0) {
      throw std::runtime_error("n must be non-negative.");
    }
    n = state.range(0);
    inputVars.resize(n, propagation::NULL_ID);
    std::vector<propagation::VarId> violationVars(n - 1, propagation::NULL_ID);
    // number of constraints: n
    // total number of static input variables: 3 * n
    engine->open();

    setEngineModes(*engine, state.range(1));

    for (size_t i = 0; i < n; ++i) {
      assert(i < inputVars.size());
      inputVars[i] = engine->makeIntVar(static_cast<Int>(i), 0, n - 1);
    }
    // Creating n - 1 invariants, each having two inputs and one output
    for (size_t i = 0; i < n - 1; ++i) {
      assert(i < violationVars.size());
      violationVars[i] = engine->makeIntVar(static_cast<Int>(i), 0, n - 1);
      assert(i + 1 < inputVars.size());
      engine->makeInvariant<propagation::AbsDiff>(
          *engine, violationVars[i], inputVars[i], inputVars[i + 1]);
    }

    totalViolation = engine->makeIntVar(0, 0, n);
    // Creating one invariant, taking n input variables and one output
    engine->makeConstraint<propagation::AllDifferent>(*engine, totalViolation,
                                                      violationVars);

    engine->close();

    gen = std::mt19937(rd());

    distribution =
        std::uniform_int_distribution<Int>{0, static_cast<Int>(n) - 1};
  }

  void TearDown(const ::benchmark::State&) override { inputVars.clear(); }
};

BENCHMARK_DEFINE_F(AllInterval, probe_single_swap)(::benchmark::State& st) {
  Int probes = 0;
  for (auto _ : st) {
    const size_t i = distribution(gen);
    assert(i < inputVars.size());
    const size_t j = distribution(gen);
    assert(j < inputVars.size());
    const Int oldI = engine->committedValue(inputVars[i]);
    const Int oldJ = engine->committedValue(inputVars[j]);
    // Perform random swap
    engine->beginMove();
    engine->setValue(inputVars[i], oldJ);
    engine->setValue(inputVars[j], oldI);
    engine->endMove();

    engine->beginProbe();
    engine->query(totalViolation);
    engine->endProbe();
    ++probes;
    assert(all_in_range(0, n - 1, [&](const size_t a) {
      return all_in_range(a + 1, n, [&](const size_t b) {
        return engine->committedValue(inputVars.at(a)) !=
                   engine->committedValue(inputVars.at(b)) &&
               engine->currentValue(inputVars.at(a)) !=
                   engine->currentValue(inputVars.at(b));
      });
    }));
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(AllInterval, probe_all_swap)(::benchmark::State& st) {
  Int probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
        const Int oldI = engine->committedValue(inputVars[i]);
        const Int oldJ = engine->committedValue(inputVars[j]);
        engine->beginMove();
        engine->setValue(inputVars[i], oldJ);
        engine->setValue(inputVars[j], oldI);
        engine->endMove();

        engine->beginProbe();
        engine->query(totalViolation);
        engine->endProbe();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(AllInterval, commit_single_swap)(::benchmark::State& st) {
  int commits = 0;
  for (auto _ : st) {
    const size_t i = distribution(gen);
    assert(i < inputVars.size());
    const size_t j = distribution(gen);
    assert(j < inputVars.size());
    const Int oldI = engine->committedValue(inputVars[i]);
    const Int oldJ = engine->committedValue(inputVars[j]);
    // Perform random swap
    engine->beginMove();
    engine->setValue(inputVars[i], oldJ);
    engine->setValue(inputVars[j], oldI);
    engine->endMove();

    engine->beginCommit();
    engine->query(totalViolation);
    engine->endCommit();

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