#include <benchmark/benchmark.h>

#include <cassert>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "constraints/allDifferent.hpp"
#include "constraints/equal.hpp"
#include "constraints/lessThan.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/linear.hpp"

class GolombRuler : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<> distribution;

  int markCount;
  int ub;

  std::vector<VarId> marks;
  std::vector<VarId> differences;

  std::vector<VarId> violations;
  VarId totalViolation;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();

    markCount = state.range(0);
    ub = markCount * markCount;

    int pairCount = markCount * (markCount - 1) / 2;
    engine->open();
    setEngineModes(*engine, state.range(1));

    // Let first mark equal 0
    marks.emplace_back(engine->makeIntVar(0, 0, 0));
    Int prevVal = 0;
    for (int i = 1; i < markCount; ++i) {
      const Int markLb = i;
      const Int markUb = ub - (markCount - i + 1);

      const Int markVal = std::uniform_int_distribution<size_t>(
          std::max<Int>(prevVal + 1, markLb), markUb)(gen);

      marks.emplace_back(engine->makeIntVar(markVal, markLb, markUb));
      prevVal = markVal;
    }

    // strictly_increasing(marks): This is the implicit constraint!
    /*
    for (int i = 1; i < markCount; ++i) {
      engine->makeConstraint<LessThan>(
          violations.emplace_back(engine->makeIntVar(0, 0, ub + 1)),
          marks.at(i - 1), marks.at(i));
    }
    */

    // array[1..pairCount] of var 0..ub: differences = [ marks[j] - marks[i]
    // | i in 1..markCount, j in i+1..markCount];
    for (int i = 0; i < markCount; ++i) {
      for (int j = i + 1; j < markCount; ++j) {
        std::vector<Int> coef{1, -1};
        std::vector<VarId> vars{marks.at(j), marks.at(i)};

        engine->makeInvariant<Linear>(
            differences.emplace_back(engine->makeIntVar(0, 0, ub)), coef, vars);
      }
    }

    // differences must be unique
    totalViolation =
        engine->makeIntVar(0, 0, pairCount + 2 * ub + pairCount * ub);
    engine->makeConstraint<AllDifferent>(
        // violations.emplace_back(engine->makeIntVar(0, 0, pairCount - 1)),
        totalViolation, differences);

    // Sum violations
    // engine->makeInvariant<Linear>(totalViolation, violations);

    engine->close();

    gen = std::mt19937(rd());
    distribution = std::uniform_int_distribution<>{1, markCount - 2};
  }

  void TearDown(const ::benchmark::State&) {
    marks.clear();
    differences.clear();
  }
};

BENCHMARK_DEFINE_F(GolombRuler, probe_single)(benchmark::State& st) {
  size_t probes = 0;
  std::uniform_int_distribution<size_t> markDist(1, marks.size() - 1);

  std::vector<size_t> indices(marks.size() - 1);
  std::iota(indices.begin(), indices.end(), 1);
  auto rng = std::default_random_engine{};
  const size_t lastIndex = marks.size() - 1;

  for (auto _ : st) {
    st.PauseTiming();
    assert(std::all_of(indices.begin(), indices.end(), [&](const size_t i) {
      return engine->committedValue(marks.at(i - 1)) <
             engine->committedValue(marks.at(i));
    }));
    std::shuffle(std::begin(indices), std::end(indices), rng);
    st.ResumeTiming();

    for (const size_t i : indices) {
      const Int markLb =
          std::max<Int>(engine->lowerBound(marks[i]),
                        engine->committedValue(marks[i - 1]) + 1);
      const Int markUb =
          i == lastIndex
              ? engine->upperBound(marks[i])
              : std::min<Int>(engine->upperBound(marks[i]),
                              engine->committedValue(marks[i + 1]) - 1);

      assert(markLb <= markUb);
      assert(engine->lowerBound(marks.at(i)) <= markLb);
      assert(engine->upperBound(marks.at(i)) >= markUb);
      assert(engine->committedValue(marks.at(i - 1)) < markLb);
      assert(i == lastIndex ||
             engine->committedValue(marks.at(i + 1)) > markUb);

      if (markLb == markUb) {
        continue;
      }

      const Int newVal =
          std::uniform_int_distribution<Int>(markLb, markUb - 1)(gen);

      engine->beginMove();
      engine->setValue(marks[i], newVal == engine->committedValue(marks[i])
                                     ? markUb
                                     : newVal);
      engine->endMove();

      engine->beginProbe();
      engine->query(totalViolation);
      engine->endProbe();

      ++probes;
      break;
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

//*

static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int markCount = 6; markCount <= 20; markCount += 2) {
    for (int mode = 0; mode <= 3; ++mode) {
      benchmark->Args({markCount, mode});
    }
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(GolombRuler, probe_single)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);

//*/