#include <benchmark/benchmark.h>

#include <exception>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "constraints/allDifferent.hpp"
#include "constraints/equal.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/absDiff.hpp"
#include "invariants/linear.hpp"

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

    n = state.range(0);
    if (n < 0) {
      throw std::runtime_error("n must be non-negative.");
    }
    Int n2 = n * n;
    gen = std::mt19937(rd());

    Int magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<Int>{0, n2 - 1};

    engine->open();

    setEngineModes(*engine, state.range(1));

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
        engine->makeInvariant<Linear>(rowSum, ones, square[i]);
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
        engine->makeInvariant<Linear>(colSum, ones, col);
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
      engine->makeInvariant<Linear>(downDiagSum, ones, diag);
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
      engine->makeInvariant<Linear>(upDiagSum, ones, diag);
      engine->makeConstraint<Equal>(upDiagViol, upDiagSum, magicSumVar);
      violations.push_back(upDiagViol);
    }

    std::vector<Int> ones{};
    ones.assign(violations.size(), 1);
    totalViolation = engine->makeIntVar(0, 0, n2 * n2 * 2 + 2 * n2);
    engine->makeInvariant<Linear>(totalViolation, ones, violations);
    engine->close();
  }

  void TearDown(const ::benchmark::State&) override {
    square.clear();
    flat.clear();
  }
};

BENCHMARK_DEFINE_F(MagicSquare, probe_single_swap)(benchmark::State& st) {
  for (auto _ : st) {
    const size_t i = distribution(gen);
    assert(i < flat.size());
    const size_t j = distribution(gen);
    assert(j < flat.size());

    const Int oldI = engine->committedValue(flat[i]);
    const Int oldJ = engine->committedValue(flat[j]);
    engine->beginMove();
    engine->setValue(flat[i], oldJ);
    engine->setValue(flat[j], oldI);
    engine->endMove();

    engine->beginProbe();
    engine->query(totalViolation);
    engine->endProbe();
  }
}

BENCHMARK_DEFINE_F(MagicSquare, probe_all_swap)(benchmark::State& st) {
  int probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
        const Int oldI = engine->committedValue(flat[i]);
        const Int oldJ = engine->committedValue(flat[j]);
        engine->beginMove();
        engine->setValue(flat[i], oldJ);
        engine->setValue(flat[j], oldI);
        engine->endMove();

        engine->beginProbe();
        engine->query(totalViolation);
        engine->endProbe();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

//*
static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int n = 4; n <= 16; n += 2) {
    for (int mode = 0; mode <= 3; ++mode) {
      benchmark->Args({n, mode});
    }
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(MagicSquare, probe_single_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);

//*/

/*
BENCHMARK_REGISTER_F(MagicSquare, probe_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
//*/