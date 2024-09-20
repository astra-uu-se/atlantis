#include <benchmark/benchmark.h>

#include <exception>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/absDiff.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class MagicSquare : public ::benchmark::Fixture {
 public:
  std::shared_ptr<propagation::Solver> solver;
  std::vector<std::vector<propagation::VarViewId>> square;
  std::vector<propagation::VarViewId> flat;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n{0};

  propagation::VarViewId totalViolation = propagation::NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_shared<propagation::Solver>();

    n = state.range(0);
    if (n < 0) {
      throw std::runtime_error("n must be non-negative.");
    }
    Int n2 = n * n;
    gen = std::mt19937(rd());

    const Int magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<Int>{0, n2 - 1};

    solver->open();

    setSolverMode(*solver, static_cast<int>(state.range(1)));

    square.resize(n, std::vector<propagation::VarViewId>{});
    flat.reserve(n * n);

    for (Int i = 0; i < n; ++i) {
      square[i].reserve(n);
      for (Int j = 0; j < n; ++j) {
        const auto var = solver->makeIntVar(i * n + j + 1, 1, n2);
        square[i].emplace_back(var);
        flat.emplace_back(var);
      }
    }
    assert(static_cast<size_t>(n) == square.size());
    assert(static_cast<size_t>(n * n) == flat.size());

    std::vector<propagation::VarViewId> violations;

    // Row
    for (Int i = 0; i < n; ++i) {
      const propagation::VarViewId rowSum = solver->makeIntVar(0, 0, n2 * n);
      solver->makeInvariant<propagation::Linear>(
          *solver, rowSum, std::vector<propagation::VarViewId>(square[i]));
      violations.emplace_back(solver->makeIntView<propagation::EqualConst>(
          *solver, rowSum, magicSum));
    }

    // Column
    for (Int i = 0; i < n; ++i) {
      const propagation::VarViewId colSum = solver->makeIntVar(0, 0, n2 * n);
      std::vector<propagation::VarViewId> col;
      col.reserve(n);
      for (Int j = 0; j < n; ++j) {
        assert(square[j].size() == static_cast<size_t>(n));
        col.emplace_back(square[j][i]);
      }
      solver->makeInvariant<propagation::Linear>(*solver, colSum,
                                                 std::move(col));
      violations.emplace_back(solver->makeIntView<propagation::EqualConst>(
          *solver, colSum, magicSum));
    }

    // downDiag
    const propagation::VarViewId downDiagSum = solver->makeIntVar(0, 0, n2 * n);
    std::vector<propagation::VarViewId> downDiag;
    downDiag.reserve(n);
    for (Int j = 0; j < n; ++j) {
      assert(square[j].size() == static_cast<size_t>(n));
      downDiag.emplace_back(square[j][j]);
    }
    solver->makeInvariant<propagation::Linear>(*solver, downDiagSum,
                                               std::move(downDiag));
    violations.emplace_back(solver->makeIntView<propagation::EqualConst>(
        *solver, downDiagSum, magicSum));

    // upDiag
    const propagation::VarViewId upDiagSum = solver->makeIntVar(0, 0, n2 * n);
    std::vector<propagation::VarViewId> upDiag;
    upDiag.reserve(n);
    for (Int j = 0; j < n; ++j) {
      assert(square[n - j - 1].size() == static_cast<size_t>(n));
      upDiag.emplace_back(square[n - j - 1][j]);
    }
    solver->makeInvariant<propagation::Linear>(*solver, upDiagSum,
                                               std::move(upDiag));
    violations.emplace_back(solver->makeIntView<propagation::EqualConst>(
        *solver, upDiagSum, magicSum));

    // total violation
    assert(2 + 2 * static_cast<size_t>(n) == violations.size());
    Int maxViol = 0;
    for (propagation::VarViewId viol : violations) {
      maxViol += solver->upperBound(viol);
    }

    totalViolation = solver->makeIntVar(0, 0, maxViol);
    solver->makeInvariant<propagation::Linear>(*solver, totalViolation,
                                               std::move(violations));
    solver->close();
  }

  void TearDown(const ::benchmark::State&) override {
    square.clear();
    flat.clear();
  }

  [[nodiscard]] inline bool sanity() const {
    return all_in_range(0, flat.size() - 1, [&](const size_t i) {
      return all_in_range(i + 1, flat.size(), [&](const size_t j) {
        return solver->committedValue(flat.at(i)) !=
                   solver->committedValue(flat.at(j)) &&
               solver->currentValue(flat.at(i)) !=
                   solver->currentValue(flat.at(j));
      });
    });
  }
};

BENCHMARK_DEFINE_F(MagicSquare, probe_single_swap)(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    const size_t i = distribution(gen);
    assert(i < flat.size());
    const size_t j = distribution(gen);
    assert(j < flat.size());

    const Int oldI = solver->committedValue(flat[i]);
    const Int oldJ = solver->committedValue(flat[j]);
    solver->beginMove();
    solver->setValue(flat[i], oldJ);
    solver->setValue(flat[j], oldI);
    solver->endMove();

    solver->beginProbe();
    solver->query(totalViolation);
    solver->endProbe();
    assert(sanity());
    ++probes;
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(MagicSquare, probe_all_swap)(::benchmark::State& st) {
  int probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
        const Int oldI = solver->committedValue(flat[i]);
        const Int oldJ = solver->committedValue(flat[j]);
        solver->beginMove();
        solver->setValue(flat[i], oldJ);
        solver->setValue(flat[j], oldI);
        solver->endMove();

        solver->beginProbe();
        solver->query(totalViolation);
        solver->endProbe();

        ++probes;
        assert(sanity());
      }
    }
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

//*
BENCHMARK_REGISTER_F(MagicSquare, probe_single_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/

/*
BENCHMARK_REGISTER_F(MagicSquare, probe_all_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/
}  // namespace atlantis::benchmark
