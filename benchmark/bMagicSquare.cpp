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
  size_t n;

  VarId totalViolation = NULL_ID;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();

    n = state.range(1);
    int n2 = n * n;
    gen = std::mt19937(rd());

    int magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<>{0, n2 - 1};

    engine->open();

    switch (state.range(0)) {
      case 0:
        engine->setPropagationMode(
            PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
        break;
      case 1:
        engine->setPropagationMode(PropagationEngine::PropagationMode::MIXED);
        break;
      case 2:
        engine->setPropagationMode(
            PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
        break;
    }

    VarId magicSumVar = engine->makeIntVar(magicSum, magicSum, magicSum);

    for (size_t i = 0; i < n; ++i) {
      square.push_back(std::vector<VarId>{});
      for (size_t j = 0; j < n; ++j) {
        const auto var = engine->makeIntVar(i * n + j + 1, 1, n2);
        square[i].push_back(var);
        flat.push_back(var);
      }
    }
    assert(n == square.size());
    assert(n * n == flat.size());

    std::vector<VarId> violations;

    {
      // Row
      std::vector<Int> ones{};
      ones.assign(n, 1);
      for (size_t i = 0; i < n; ++i) {
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
      for (size_t i = 0; i < n; ++i) {
        const VarId colSum = engine->makeIntVar(0, 0, n2 * n);
        const VarId colViol = engine->makeIntVar(0, 0, n2 * n);
        std::vector<VarId> col{};
        for (size_t j = 0; j < n; ++j) {
          assert(square[j].size() == n);
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
      for (size_t j = 0; j < n; ++j) {
        assert(square[j].size() == n);
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
      for (size_t j = 0; j < n; ++j) {
        assert(square[n - j - 1].size() == n);
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

  void TearDown(const ::benchmark::State&) {
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
    for (int mode = 0; mode <= 2; ++mode) {
      benchmark->Args({mode, n});
    }
  }
}

BENCHMARK_REGISTER_F(MagicSquare, probing_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
//*/