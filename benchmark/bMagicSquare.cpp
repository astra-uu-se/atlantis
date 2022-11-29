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
#include "views/equalConst.hpp"

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

    const Int magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<Int>{0, n2 - 1};

    engine->open();

    setEngineModes(*engine, state.range(1));

    square.reserve(n);
    flat.reserve(n * n);

    for (Int i = 0; i < n; ++i) {
      square.push_back(std::vector<VarId>(n));
      for (Int j = 0; j < n; ++j) {
        const auto var = engine->makeIntVar(i * n + j + 1, 1, n2);
        square[i].at(j) = var;
        flat.push_back(var);
      }
    }
    assert(static_cast<size_t>(n) == square.size());
    assert(static_cast<size_t>(n * n) == flat.size());

    std::vector<VarId> violations;

    // Row
    for (Int i = 0; i < n; ++i) {
      const VarId rowSum = engine->makeIntVar(0, 0, n2 * n);
      engine->makeInvariant<Linear>(*engine, rowSum, square[i]);
      violations.push_back(
          engine->makeIntView<EqualConst>(*engine, rowSum, magicSum));
    }

    // Column
    for (Int i = 0; i < n; ++i) {
      const VarId colSum = engine->makeIntVar(0, 0, n2 * n);
      std::vector<VarId> col(n);
      for (Int j = 0; j < n; ++j) {
        assert(square[j].size() == static_cast<size_t>(n));
        col.at(j) = square[j][i];
      }
      engine->makeInvariant<Linear>(*engine, colSum, col);
      violations.push_back(
          engine->makeIntView<EqualConst>(*engine, colSum, magicSum));
    }

    // downDiag
    const VarId downDiagSum = engine->makeIntVar(0, 0, n2 * n);
    std::vector<VarId> downDiag(n);
    for (Int j = 0; j < n; ++j) {
      assert(square[j].size() == static_cast<size_t>(n));
      downDiag.at(j) = square[j][j];
    }
    engine->makeInvariant<Linear>(*engine, downDiagSum, downDiag);
    violations.push_back(
        engine->makeIntView<EqualConst>(*engine, downDiagSum, magicSum));

    // upDiag
    const VarId upDiagSum = engine->makeIntVar(0, 0, n2 * n);
    std::vector<VarId> upDiag(n);
    for (Int j = 0; j < n; ++j) {
      assert(square[n - j - 1].size() == static_cast<size_t>(n));
      upDiag.at(j) = square[n - j - 1][j];
    }
    engine->makeInvariant<Linear>(*engine, upDiagSum, upDiag);
    violations.push_back(
        engine->makeIntView<EqualConst>(*engine, upDiagSum, magicSum));

    // total violation
    assert(2 + 2 * static_cast<size_t>(n) == violations.size());
    Int maxViol = 0;
    for (VarId viol : violations) {
      maxViol += engine->upperBound(viol);
    }

    totalViolation = engine->makeIntVar(0, 0, maxViol);
    engine->makeInvariant<Linear>(*engine, totalViolation, violations);
    engine->close();
  }

  void TearDown(const ::benchmark::State&) override {
    square.clear();
    flat.clear();
  }

  inline bool sanity() const {
    return all_in_range(0, flat.size() - 1, [&](const size_t i) {
      return all_in_range(i + 1, flat.size(), [&](const size_t j) {
        return engine->committedValue(flat.at(i)) !=
                   engine->committedValue(flat.at(j)) &&
               engine->currentValue(flat.at(i)) !=
                   engine->currentValue(flat.at(j));
      });
    });
  }
};

BENCHMARK_DEFINE_F(MagicSquare, probe_single_swap)(benchmark::State& st) {
  size_t probes = 0;
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
    assert(sanity());
    ++probes;
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
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
        assert(sanity());
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

//*
BENCHMARK_REGISTER_F(MagicSquare, probe_single_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/

/*
BENCHMARK_REGISTER_F(MagicSquare, probe_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/