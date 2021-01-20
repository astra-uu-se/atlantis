#include <benchmark/benchmark.h>

#include <constraints/allDifferent.hpp>
#include <core/propagationEngine.hpp>
#include <invariants/absDiff.hpp>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

class AllInterval : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<VarId> s_vars;
  std::vector<VarId> v_vars;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<> distribution;
  int n;

  VarId violation = NULL_ID;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();
    n = state.range(0);

    // TODO: why is this printed multiple times per test?
    logDebug(n);
    engine->open();

    for (int i = 0; i < n; ++i) {
      s_vars.push_back(engine->makeIntVar(i, 0, n - 1));
    }

    for (int i = 1; i < n; ++i) {
      v_vars.push_back(engine->makeIntVar(i, 0, n - 1));
      engine->makeInvariant<AbsDiff>(s_vars.at(i - 1), s_vars.at(i),
                                     v_vars.back());
    }

    violation = engine->makeIntVar(0, 0, n);
    engine->makeConstraint<AllDifferent>(violation, v_vars);

    engine->close();

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) {
    s_vars.clear();
    v_vars.clear();
  }
};

BENCHMARK_DEFINE_F(AllInterval, probing_single_swap)(benchmark::State& st) {
  for (auto _ : st) {
    int i = distribution(gen);
    int j = distribution(gen);
    Int oldI = engine->getCommittedValue(s_vars.at(i));
    Int oldJ = engine->getCommittedValue(s_vars.at(j));
    // Perform random swap
    engine->beginMove();
    engine->setValue(s_vars.at(i), oldJ);
    engine->setValue(s_vars.at(j), oldI);
    engine->endMove();

    engine->beginQuery();
    engine->query(violation);
    engine->endQuery();
  }
}

BENCHMARK_DEFINE_F(AllInterval, probing_all_swap)(benchmark::State& st) {
  Int probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
        Int oldI = engine->getCommittedValue(s_vars.at(i));
        Int oldJ = engine->getCommittedValue(s_vars.at(j));
        engine->beginMove();
        engine->setValue(s_vars.at(i), oldJ);
        engine->setValue(s_vars.at(j), oldI);
        engine->endMove();

        engine->beginQuery();
        engine->query(violation);
        engine->endQuery();

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
    int i = distribution(gen);
    int j = distribution(gen);

    Int oldI = engine->getCommittedValue(s_vars.at(i));
    Int oldJ = engine->getCommittedValue(s_vars.at(j));
    // Perform random swap
    engine->beginMove();
    engine->setValue(s_vars.at(i), oldJ);
    engine->setValue(s_vars.at(j), oldI);
    engine->endMove();

    engine->beginCommit();
    engine->query(violation);
    engine->endCommit();

    ++commits;
  }

  st.counters["seconds_per_commit"] = benchmark::Counter(
      commits, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
//

BENCHMARK_REGISTER_F(AllInterval, probing_single_swap)->DenseRange(5, 30, 5);
BENCHMARK_REGISTER_F(AllInterval, probing_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(5, 30, 5);
BENCHMARK_REGISTER_F(AllInterval, commit_single_swap)->DenseRange(5, 30, 5);
