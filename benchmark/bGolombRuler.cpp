#include <benchmark/benchmark.h>

#include <cassert>
#include <constraints/allDifferent.hpp>
#include <constraints/equal.hpp>
#include <constraints/lessThan.hpp>
#include <core/propagationEngine.hpp>
#include <invariants/linear.hpp>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

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

    markCount = state.range(1);
    ub = markCount * markCount;

    int pairCount = markCount * (markCount - 1) / 2;
    engine->open();
    engine->setPropagationMode(
        static_cast<PropagationEngine::PropagationMode>(state.range(0)));

    for (int i = 0; i < markCount; ++i) {
      marks.push_back(engine->makeIntVar(0, 0, ub));
    }

    for (int i = 0; i < pairCount; ++i) {
      differences.push_back(engine->makeIntVar(0, 0, ub));
    }

    // Let first marks equal 0
    VarId constZero = engine->makeIntVar(0, 0, 0);
    violations.push_back(engine->makeIntVar(0, 0, ub));
    engine->makeConstraint<Equal>(violations.back(), marks.front(), constZero);

    // marks[i-1] < marks[i] forall i in 1..markCount-1
    for (int i = 1; i < markCount; ++i) {
      violations.push_back(engine->makeIntVar(0, 0, ub + 1));
      engine->makeConstraint<LessThan>(violations.back(), marks.at(i - 1),
                                       marks.at(i));
    }

    // differences must be unique
    violations.push_back(engine->makeIntVar(0, 0, pairCount - 1));
    engine->makeConstraint<AllDifferent>(violations.back(), differences);

    // array[1..pairCount] of var 0..ub: differences = [ marks[j] - marks[i]
    // | i in 1..markCount, j in i+1..markCount];
    std::vector<VarId> pairDiff;
    int it = 0;
    for (int i = 0; i < markCount; ++i) {
      for (int j = i + 1; j < markCount; ++j) {
        pairDiff.push_back(engine->makeIntVar(0, 0, ub));

        std::vector<Int> coef{1, -1};
        std::vector<VarId> vars{marks.at(j), marks.at(i)};

        engine->makeInvariant<Linear>(coef, vars, pairDiff.back());

        violations.push_back(engine->makeIntVar(0, 0, ub));
        engine->makeConstraint<Equal>(violations.back(), differences.at(it),
                                      pairDiff.back());

        ++it;
      }
    }

    // Sanity check that we post the contraint for each
    // unique pair of variables in marks
    assert(it == pairCount);

    // Sum violations
    totalViolation =
        engine->makeIntVar(0, 0, pairCount + 2 * ub + pairCount * ub);
    engine->makeInvariant<Linear>(violations, totalViolation);

    engine->close();

    gen = std::mt19937(rd());
    distribution = std::uniform_int_distribution<>{1, markCount - 2};
  }

  void TearDown(const ::benchmark::State&) {
    marks.clear();
    differences.clear();
  }
};
