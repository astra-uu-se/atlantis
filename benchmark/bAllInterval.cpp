#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/absDiff.hpp"

class AllInterval : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<VarId> inputVars;
  std::vector<VarId> violationVars;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n;

  VarId totalViolation = NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    engine = std::make_unique<PropagationEngine>();
    n = state.range(0);
    if (n < 0) {
      throw std::runtime_error("n must be non-negative.");
    }

    engine->open();

    setEngineModes(*engine, state.range(1));

    for (int i = 0; i < n; ++i) {
      inputVars.push_back(engine->makeIntVar(i, 0, n - 1));
    }

    for (int i = 1; i < n; ++i) {
      violationVars.push_back(engine->makeIntVar(i, 0, n - 1));
      engine->makeInvariant<AbsDiff>(violationVars.back(), inputVars[i - 1],
                                     inputVars[i]);
    }

    totalViolation = engine->makeIntVar(0, 0, n);
    engine->makeConstraint<AllDifferent>(totalViolation, violationVars);

    engine->close();

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<Int>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    inputVars.clear();
    violationVars.clear();
  }
};

BENCHMARK_DEFINE_F(AllInterval, probe_single_swap)(benchmark::State& st) {
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
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(AllInterval, probe_all_swap)(benchmark::State& st) {
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
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(AllInterval, commit_single_swap)(benchmark::State& st) {
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

  st.counters["seconds_per_commit"] = benchmark::Counter(
      commits, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

//*
static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int n = 10; n <= 100; n += 10) {
    for (int mode = 0; mode <= 3; ++mode) {
      benchmark->Args({n, mode});
    }
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(AllInterval, probe_single_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
//*/
/*
BENCHMARK_REGISTER_F(AllInterval, probe_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);

//*/
/*

static void commitArguments(benchmark::internal::Benchmark* benchmark) {
  for (int n = 10; n <= 10; n += 10) {
    benchmark->Args({n, 0});
  }
#ifndef NDEBUG
    break;
#endif
}
BENCHMARK_REGISTER_F(AllInterval, commit_single_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(commitArguments);
//*/