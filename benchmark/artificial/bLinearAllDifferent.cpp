#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

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
    bool overlappingLinears = state.range(1) != 0;
    std::vector<VarId> linearOutputVars;
    size_t increment;

    if (overlappingLinears) {
      varCount = state.range(2);
      linearOutputVars.reserve(varCount - 1);
      increment = 1;
    } else {
      varCount = state.range(2) - (state.range(2) % 2);
      linearOutputVars.reserve(varCount / 2);
      increment = 2;
    }

    engine->open();
    engine->setPropagationMode(
        static_cast<PropagationEngine::PropagationMode>(state.range(0)));

    decisionVars.reserve(varCount);

    for (size_t i = 0; i < varCount; ++i) {
      decisionVars.push_back(engine->makeIntVar(i, 0, varCount - 1));
    }

    for (size_t i = 0; i < varCount - 1; i += increment) {
      linearOutputVars.push_back(engine->makeIntVar(i, 0, 2 * (varCount - 1)));
      engine->makeInvariant<Linear>(
          std::vector<VarId>{decisionVars.at(i), decisionVars.at(i + 1)},
          linearOutputVars.back());
    }

    violation = engine->makeIntVar(0, 0, varCount);
    engine->makeConstraint<AllDifferent>(violation, linearOutputVars);

    engine->close();

    gen = std::mt19937(rd());

    decionVarIndexDist = std::uniform_int_distribution<size_t>{0, varCount - 1};
  }

  void TearDown(const ::benchmark::State&) { decisionVars.clear(); }
};

BENCHMARK_DEFINE_F(LinearAllDifferent, probing_single_swap)
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

BENCHMARK_DEFINE_F(LinearAllDifferent, probing_all_swap)
(benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < varCount; ++i) {
      for (size_t j = i + 1; j < varCount; ++j) {
        engine->beginMove();
        engine->setValue(decisionVars.at(i),
                         engine->getCommittedValue(decisionVars.at(j)));
        engine->setValue(decisionVars.at(j),
                         engine->getCommittedValue(decisionVars.at(i)));
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

///*

static void arguments(benchmark::internal::Benchmark* b) {
  for (int overlapping = 0; overlapping <= 1; ++overlapping) {
    for (int varCount = 8; varCount <= 32; varCount *= 2) {
      for (Int mode = 0; mode <= 2; ++mode) {
        b->Args({mode, overlapping, varCount});
      }
    }
  }
}

BENCHMARK_REGISTER_F(LinearAllDifferent, probing_single_swap)->Apply(arguments);

BENCHMARK_REGISTER_F(LinearAllDifferent, probing_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
/*
BENCHMARK_REGISTER_F(LinearAllDifferent, commit_single_swap)->Apply(arguments);
//*/