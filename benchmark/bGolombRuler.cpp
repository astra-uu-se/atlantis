#include <benchmark/benchmark.h>

#include <cassert>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "propagation/constraints/allDifferent.hpp"
#include "propagation/constraints/equal.hpp"
#include "propagation/constraints/lessThan.hpp"
#include "propagation/invariants/absDiff.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::benchmark {

class GolombRuler : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  std::mt19937 gen;

  size_t markCount;

  std::vector<propagation::VarId> marks;
  std::vector<propagation::VarId> differences;

  std::vector<propagation::VarId> violations;
  propagation::VarId totalViolation;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<propagation::PropagationEngine>();

    markCount = state.range(0);
    const size_t ub = markCount * markCount;
    std::random_device rd;
    gen = std::mt19937(rd());

    engine->open();
    setEngineModes(*engine, state.range(1));

    // Let first mark equal 0
    marks.emplace_back(engine->makeIntVar(0, 0, 0));
    Int prevVal = 0;
    for (size_t i = 1; i < markCount; ++i) {
      const Int markLb = i;
      const Int markUb = static_cast<Int>(ub) - (markCount - i + 1);

      const Int markVal = std::uniform_int_distribution<size_t>(
          std::max<Int>(prevVal + 1, markLb), markUb)(gen);

      marks.emplace_back(engine->makeIntVar(markVal, markLb, markUb));
      prevVal = markVal;
    }

    differences.reserve(((markCount - 1) * (markCount)) / 2);

    // creating invariants quadratic in markCount
    // each with two edges, resulting in a total
    // number of edges quadratic in markCount
    for (size_t i = 0; i < markCount; ++i) {
      for (size_t j = i + 1; j < markCount; ++j) {
        engine->makeInvariant<propagation::AbsDiff>(
            *engine,
            differences.emplace_back(
                engine->makeIntVar(0, 0, static_cast<Int>(ub))),
            marks.at(i), marks.at(j));
      }
    }
    assert(differences.size() == ((markCount - 1) * (markCount)) / 2);

    Int maxViol = 0;
    for (propagation::VarId viol : differences) {
      maxViol += engine->upperBound(viol);
    }

    // differences must be unique
    totalViolation = engine->makeIntVar(0, 0, maxViol);
    engine->makeConstraint<propagation::AllDifferent>(*engine, totalViolation,
                                                      differences);

    engine->close();
  }

  void TearDown(const ::benchmark::State&) {
    marks.clear();
    differences.clear();
  }
};

BENCHMARK_DEFINE_F(GolombRuler, probe_single)(::benchmark::State& st) {
  size_t probes = 0;

  std::vector<size_t> indices(marks.size());
  std::iota(indices.begin(), indices.end(), 0);
  const size_t lastIndex = marks.size() - 1;

  for (auto _ : st) {
    for (size_t i = 0; i <= lastIndex; ++i) {
      assert(i <= lastIndex);

      std::swap<size_t>(
          indices[i], indices[rand_in_range(std::min<size_t>(i + 1, lastIndex),
                                            lastIndex, gen)]);
      assert(all_in_range(0, indices.size() - 1, [&](const size_t a) {
        return all_in_range(a + 1, indices.size(),
                            [&](const size_t b) { return a != b; });
      }));
      const size_t index = indices[i];
      assert(index <= lastIndex);

      const Int markLb =
          index == 0
              ? engine->lowerBound(marks[index])
              : std::max<Int>(engine->lowerBound(marks[index]),
                              engine->committedValue(marks[index - 1]) + 1);
      const Int markUb =
          index == lastIndex
              ? engine->upperBound(marks[index])
              : std::min<Int>(engine->upperBound(marks[index]),
                              engine->committedValue(marks[index + 1]) - 1);

      assert(markLb <= markUb);
      assert(engine->lowerBound(marks.at(index)) <= markLb);
      assert(engine->upperBound(marks.at(index)) >= markUb);
      assert(index == 0 ||
             engine->committedValue(marks.at(index - 1)) < markLb);
      assert(index == lastIndex ||
             engine->committedValue(marks.at(index + 1)) > markUb);

      if (markLb == markUb) {
        continue;
      }

      const Int newVal = rand_in_range(markLb, markUb - 1, gen);

      engine->beginMove();
      engine->setValue(
          marks[index],
          newVal == engine->committedValue(marks[index]) ? markUb : newVal);
      engine->endMove();

      engine->beginProbe();
      engine->query(totalViolation);
      engine->endProbe();

      ++probes;
      break;
    }
    assert(all_in_range(1, marks.size(), [&](const size_t i) {
      return engine->committedValue(marks.at(i - 1)) <
                 engine->committedValue(marks.at(i)) &&
             engine->currentValue(marks.at(i - 1)) <
                 engine->currentValue(marks.at(i));
    }));
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

//*
BENCHMARK_REGISTER_F(GolombRuler, probe_single)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/
}  // namespace atlantis::benchmark