#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/absDiff.hpp"
#include "invariants/linear.hpp"
#include "misc/logging.hpp"

class LinearAllDifferent : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<VarId> decisionVars;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<size_t> decionVarIndexDist;
  size_t varCount;

  VarId violation = NULL_ID;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();
    bool overlappingLinears = state.range(0) != 0;
    std::vector<VarId> linearOutputVars;
    size_t increment;

    if (overlappingLinears) {
      varCount = state.range(1);
      linearOutputVars.reserve(varCount - 1);
      increment = 1;
    } else {
      varCount = state.range(1) - (state.range(1) % 2);
      linearOutputVars.reserve(varCount / 2);
      increment = 2;
    }

    engine->open();
    setEngineModes(*engine, state.range(2));

    decisionVars.reserve(varCount);

    for (size_t i = 0; i < varCount; ++i) {
      decisionVars.push_back(engine->makeIntVar(i, 0, varCount - 1));
    }

    for (size_t i = 0; i < varCount - 1; i += increment) {
      linearOutputVars.push_back(engine->makeIntVar(i, 0, 2 * (varCount - 1)));
      engine->makeInvariant<Linear>(
          *engine.get(), linearOutputVars.back(),
          std::vector<VarId>{decisionVars.at(i), decisionVars.at(i + 1)});
    }

    violation = engine->makeIntVar(0, 0, varCount);
    engine->makeConstraint<AllDifferent>(*engine, violation, linearOutputVars);

    engine->close();

    gen = std::mt19937(rd());

    decionVarIndexDist = std::uniform_int_distribution<size_t>{0, varCount - 1};
  }

  void TearDown(const ::benchmark::State&) { decisionVars.clear(); }
};

BENCHMARK_DEFINE_F(LinearAllDifferent, probe_single_swap)
(benchmark::State& st) {
  Int probes = 0;
  for (auto _ : st) {
    size_t i = decionVarIndexDist(gen);
    size_t j = decionVarIndexDist(gen);

    // Perform random swap
    engine->beginMove();
    engine->setValue(decisionVars.at(i),
                     engine->committedValue(decisionVars.at(j)));
    engine->setValue(decisionVars.at(j),
                     engine->committedValue(decisionVars.at(i)));
    engine->endMove();

    engine->beginProbe();
    engine->query(violation);
    engine->endProbe();

    ++probes;
  }

  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(LinearAllDifferent, probe_all_swap)
(benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < varCount; ++i) {
      for (size_t j = i + 1; j < varCount; ++j) {
        engine->beginMove();
        engine->setValue(decisionVars.at(i),
                         engine->committedValue(decisionVars.at(j)));
        engine->setValue(decisionVars.at(j),
                         engine->committedValue(decisionVars.at(i)));
        engine->endMove();

        engine->beginProbe();
        engine->query(violation);
        engine->endProbe();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(LinearAllDifferent, commit_single_swap)
(benchmark::State& st) {
  size_t commits = 0;
  for (auto _ : st) {
    size_t i = decionVarIndexDist(gen);
    size_t j = decionVarIndexDist(gen);

    // Perform random swap
    engine->beginMove();
    engine->setValue(decisionVars.at(i),
                     engine->committedValue(decisionVars.at(j)));
    engine->setValue(decisionVars.at(j),
                     engine->committedValue(decisionVars.at(i)));
    engine->endMove();

    engine->beginCommit();
    engine->query(violation);
    engine->endCommit();

    ++commits;
  }

  st.counters["commits_per_second"] =
      benchmark::Counter(commits, benchmark::Counter::kIsRate);
}

/*

static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int overlapping = 0; overlapping <= 1; ++overlapping) {
    for (int varCount = 2; varCount <= 16; varCount *= 2) {
      for (Int mode = 0; mode <= 3; ++mode) {
        benchmark->Args({overlapping, varCount, mode});
      }
#ifndef NDEBUG
      return;
#endif
    }
  }
}

BENCHMARK_REGISTER_F(LinearAllDifferent, probe_single_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);

/*
BENCHMARK_REGISTER_F(LinearAllDifferent, probe_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
/*
BENCHMARK_REGISTER_F(LinearAllDifferent, commit_single_swap)->Apply(arguments);
//*/