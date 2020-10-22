#include <benchmark/benchmark.h>

#include <constraints/allDifferent.hpp>
#include <core/propagationEngine.hpp>
#include <invariants/absDiff.hpp>
#include <random>
#include <utility>
#include <vector>

class AllInterval : public benchmark::Fixture {
 public:
  PropagationEngine engine;

  std::vector<VarId> s_vars;
  std::vector<VarId> v_vars;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<> distribution;
  int n;

  VarId violation = NULL_ID;

  void SetUp(const ::benchmark::State& state) {
    n = 25;
    engine.open();

    for (int i = 0; i < n; i++) {
      s_vars.push_back(engine.makeIntVar(i, 0, n - 1));
    }

    for (int i = 1; i < n; i++) {
      v_vars.push_back(engine.makeIntVar(i, 0, n - 1));
      engine.makeInvariant<AbsDiff>(s_vars.at(i - 1), s_vars.at(i),
                                    v_vars.back());
    }

    violation = engine.makeIntVar(0, 0, n);
    engine.makeConstraint<AllDifferent>(violation, v_vars);

    engine.close();

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<>{0, n - 1};
  }

  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(AllInterval, probing_single_swap)(benchmark::State& st) {
  for (auto _ : st) {
    int i = distribution(gen);
    int j = distribution(gen);
    Int oldI = engine.getValue(s_vars.at(i));
    Int oldJ = engine.getValue(s_vars.at(j));
    // Perform random swap
    engine.beginMove();
    engine.setValue(s_vars.at(i), oldJ);
    engine.setValue(s_vars.at(j), oldI);
    engine.endMove();

    engine.beginQuery();
    engine.query(violation);
    engine.endQuery();
  }
}

BENCHMARK_F(AllInterval, commit_single_swap)(benchmark::State& st) {
  for (auto _ : st) {
    int i = distribution(gen);
    int j = distribution(gen);
    Int oldI = engine.getValue(s_vars.at(i));
    Int oldJ = engine.getValue(s_vars.at(j));
    // Perform random swap
    engine.beginMove();
    engine.setValue(s_vars.at(i), oldJ);
    engine.setValue(s_vars.at(j), oldI);
    engine.endMove();

    engine.beginCommit();
    engine.query(violation);
    engine.endCommit();
  }
}
