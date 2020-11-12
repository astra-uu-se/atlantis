#include <benchmark/benchmark.h>

#include <constraints/allDifferent.hpp>
#include <constraints/equal.hpp>
#include <core/propagationEngine.hpp>
#include <invariants/absDiff.hpp>
#include <invariants/linear.hpp>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

class MagicSquare : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<std::vector<VarId>> square;
  std::vector<VarId> flat;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<> distribution;
  int n;

  VarId totalViolation = NULL_ID;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();
    n = state.range(0);
    int n2 = n * n;
    gen = std::mt19937(rd());

    int magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<>{0, n2 - 1};

    engine->open();

    VarId magicSumVar = engine->makeIntVar(magicSum, magicSum, magicSum);

    for (int i = 0; i < n; i++) {
      square.push_back(std::vector<VarId>{});
      for (int j = 0; j < n; j++) {
        auto var = engine->makeIntVar(i * n + j + 1, 1, n2);
        square.at(i).push_back(var);
        flat.push_back(var);
      }
    }

    std::vector<VarId> violations;

    // All different is implied by initial assignment + swap moves.
    // {
    //   VarId allDiffViol = engine->makeIntVar(0, 0, n2);
    //   violations.push_back(allDiffViol);
    //   engine->makeConstraint<AllDifferent>(allDiffViol, flat);
    // }

    {
      // Row
      std::vector<Int> ones{};
      ones.assign(n, 1);
      for (int i = 0; i < n; i++) {
        VarId rowSum = engine->makeIntVar(0, 0, n2 * n);
        VarId rowViol = engine->makeIntVar(0, 0, n2 * n);

        engine->makeInvariant<Linear>(ones, square.at(i), rowSum);
        engine->makeConstraint<Equal>(rowViol, rowSum, magicSumVar);
        violations.push_back(rowViol);
      }
    }

    {
      // Column
      std::vector<Int> ones{};
      ones.assign(n, 1);
      for (int i = 0; i < n; i++) {
        VarId colSum = engine->makeIntVar(0, 0, n2 * n);
        VarId colViol = engine->makeIntVar(0, 0, n2 * n);
        std::vector<VarId> col{};
        for (int j = 0; j < n; j++) {
          col.push_back(square.at(j).at(i));
        }
        engine->makeInvariant<Linear>(ones, col, colSum);
        engine->makeConstraint<Equal>(colViol, colSum, magicSumVar);
        violations.push_back(colViol);
      }
    }

    {
      // downDiag
      std::vector<Int> ones{};
      ones.assign(n, 1);
      VarId downDiagSum = engine->makeIntVar(0, 0, n2 * n);
      VarId downDiagViol = engine->makeIntVar(0, 0, n2 * n);
      std::vector<VarId> diag{};
      for (int j = 0; j < n; j++) {
        diag.push_back(square.at(j).at(j));
      }
      engine->makeInvariant<Linear>(ones, diag, downDiagSum);
      engine->makeConstraint<Equal>(downDiagViol, downDiagSum, magicSumVar);
      violations.push_back(downDiagViol);
    }

    {
      // upDiag
      std::vector<Int> ones{};
      ones.assign(n, 1);
      VarId upDiagSum = engine->makeIntVar(0, 0, n2 * n);
      VarId upDiagViol = engine->makeIntVar(0, 0, n2 * n);
      std::vector<VarId> diag{};
      for (int j = 0; j < n; j++) {
        diag.push_back(square.at(n - j - 1).at(j));
      }
      engine->makeInvariant<Linear>(ones, diag, upDiagSum);
      engine->makeConstraint<Equal>(upDiagViol, upDiagSum, magicSumVar);
      violations.push_back(upDiagViol);
    }

    std::vector<Int> ones{};
    ones.assign(violations.size(), 1);
    totalViolation = engine->makeIntVar(0, 0, n2 * n2 * 2 + 2 * n2);
    engine->makeInvariant<Linear>(ones, violations, totalViolation);
    engine->close();
  }

  void TearDown(const ::benchmark::State&) {
    square.clear();
    flat.clear();
  }
};

BENCHMARK_DEFINE_F(MagicSquare, probing_all_swap)(benchmark::State& st) {
  int probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n * n); i++) {
      for (size_t j = i + 1; j < static_cast<size_t>(n * n); j++) {
        Int oldI = engine->getCommittedValue(flat.at(i));
        Int oldJ = engine->getCommittedValue(flat.at(j));
        engine->beginMove();
        engine->setValue(flat.at(i), oldJ);
        engine->setValue(flat.at(j), oldI);
        engine->endMove();

        engine->beginQuery();
        engine->query(totalViolation);
        engine->endQuery();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(MagicSquare, probing_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(3, 10);