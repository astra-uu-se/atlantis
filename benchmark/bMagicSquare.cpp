#include <benchmark/benchmark.h>

#include <constraints/allDifferent.hpp>
#include <constraints/equal.hpp>
#include <core/propagationEngine.hpp>
#include <exception>
#include <invariants/absDiff.hpp>
#include <invariants/linear.hpp>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"

class MagicSquare : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<std::vector<VarId>> square;
  std::vector<VarId> flat;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n;

  VarId totalViolation = NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    engine = std::make_unique<PropagationEngine>();

    n = state.range(1);
    if (n < 0) {
      throw std::runtime_error("n must be non-negative.");
    }
    Int n2 = n * n;
    gen = std::mt19937(rd());

    Int magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<Int>{0, n2 - 1};

    engine->open();

    engine->setPropagationMode(intToPropagationMode(state.range(0)));
    engine->setOutputToInputMarkingMode(
        intToOutputToInputMarkingMode(state.range(0)));

    VarId magicSumVar = engine->makeIntVar(magicSum, magicSum, magicSum);

    for (Int i = 0; i < n; ++i) {
      square.push_back(std::vector<VarId>{});
      for (Int j = 0; j < n; ++j) {
        const auto var = engine->makeIntVar(i * n + j + 1, 1, n2);
        square[i].push_back(var);
        flat.push_back(var);
      }
    }
    assert(static_cast<size_t>(n) == square.size());
    assert(static_cast<size_t>(n * n) == flat.size());

    std::vector<VarId> violations;

    {
      // Row
      std::vector<Int> ones{};
      ones.assign(n, 1);
      for (Int i = 0; i < n; ++i) {
        const VarId rowSum = engine->makeIntVar(0, 0, n2 * n);
        const VarId rowViol = engine->makeIntVar(0, 0, n2 * n);
        engine->makeInvariant<Linear>(ones, square[i], rowSum);
        engine->makeConstraint<Equal>(rowViol, rowSum, magicSumVar);
        violations.push_back(rowViol);
      }
    }

    {
      // Column
      std::vector<Int> ones{};
      ones.assign(n, 1);
      for (Int i = 0; i < n; ++i) {
        const VarId colSum = engine->makeIntVar(0, 0, n2 * n);
        const VarId colViol = engine->makeIntVar(0, 0, n2 * n);
        std::vector<VarId> col{};
        for (Int j = 0; j < n; ++j) {
          assert(square[j].size() == static_cast<size_t>(n));
          col.push_back(square[j][i]);
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
      const VarId downDiagSum = engine->makeIntVar(0, 0, n2 * n);
      const VarId downDiagViol = engine->makeIntVar(0, 0, n2 * n);
      std::vector<VarId> diag{};
      for (Int j = 0; j < n; ++j) {
        assert(square[j].size() == static_cast<size_t>(n));
        diag.push_back(square[j][j]);
      }
      engine->makeInvariant<Linear>(ones, diag, downDiagSum);
      engine->makeConstraint<Equal>(downDiagViol, downDiagSum, magicSumVar);
      violations.push_back(downDiagViol);
    }

    {
      // upDiag
      std::vector<Int> ones{};
      ones.assign(n, 1);
      const VarId upDiagSum = engine->makeIntVar(0, 0, n2 * n);
      const VarId upDiagViol = engine->makeIntVar(0, 0, n2 * n);
      std::vector<VarId> diag{};
      for (Int j = 0; j < n; ++j) {
        assert(square[n - j - 1].size() == static_cast<size_t>(n));
        diag.push_back(square[n - j - 1][j]);
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

  void TearDown(const ::benchmark::State&) override {
    square.clear();
    flat.clear();
  }
};

BENCHMARK_DEFINE_F(MagicSquare, probing_all_swap)(benchmark::State& st) {
  int probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
        const Int oldI = engine->getCommittedValue(flat[i]);
        const Int oldJ = engine->getCommittedValue(flat[j]);
        engine->beginMove();
        engine->setValue(flat[i], oldJ);
        engine->setValue(flat[j], oldI);
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

///*
static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int n = 4; n <= 10; n += 2) {
    for (int mode = 0; mode <= 3; ++mode) {
      benchmark->Args({mode, n});
    }
  }
}

BENCHMARK_REGISTER_F(MagicSquare, probing_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
//*/