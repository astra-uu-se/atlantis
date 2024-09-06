#include <gtest/gtest.h>

#include <iostream>
#include <random>
#include <vector>

#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"

namespace atlantis::testing {
class MagicSquareTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::vector<std::vector<propagation::VarViewId>> square;
  std::vector<propagation::VarViewId> flat;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n{3};

  propagation::VarViewId totalViolation = propagation::NULL_ID;

  Int magicSum = 0;
  void SetUp() override {
    _solver = std::make_unique<propagation::Solver>();
    n = 3;
    Int n2 = n * n;
    gen = std::mt19937(rd());

    magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<Int>{0, n2 - 1};

    _solver->open();

    propagation::VarViewId magicSumVar =
        _solver->makeIntVar(magicSum, magicSum, magicSum);

    for (int i = 0; i < n; ++i) {
      square.emplace_back();
      for (int j = 0; j < n; ++j) {
        auto var = _solver->makeIntVar(i * n + j + 1, 1, n2);
        square.at(i).push_back(var);
        flat.push_back(var);
      }
    }

    std::vector<propagation::VarViewId> violations;

    // All different is implied by initial assignment + swap moves.
    // {
    //   propagation::VarViewId allDiffViol = _solver->makeIntVar(0, 0, n2);
    //   violations.push_back(allDiffViol);
    //   _solver->makeViolationInvariant<propagation::AllDifferent>(*_solver,
    //   allDiffViol, flat);
    // }

    {
      // Row
      for (int i = 0; i < n; ++i) {
        propagation::VarViewId rowSum = _solver->makeIntVar(0, 0, n2 * n);
        propagation::VarViewId rowViol = _solver->makeIntVar(0, 0, n2 * n);

        _solver->makeInvariant<propagation::Linear>(
            *_solver, rowSum,
            std::vector<propagation::VarViewId>(square.at(i)));
        _solver->makeViolationInvariant<propagation::Equal>(
            *_solver, rowViol, rowSum, magicSumVar);
        violations.push_back(rowViol);
      }
    }

    {
      // Column
      for (int i = 0; i < n; ++i) {
        propagation::VarViewId colSum = _solver->makeIntVar(0, 0, n2 * n);
        propagation::VarViewId colViol = _solver->makeIntVar(0, 0, n2 * n);
        std::vector<propagation::VarViewId> col{};
        col.reserve(n);
        for (int j = 0; j < n; ++j) {
          col.push_back(square.at(j).at(i));
        }
        _solver->makeInvariant<propagation::Linear>(
            *_solver, colSum, std::vector<propagation::VarViewId>(col));
        _solver->makeViolationInvariant<propagation::Equal>(
            *_solver, colViol, colSum, magicSumVar);
        violations.push_back(colViol);
      }
    }

    {
      // downDiag
      propagation::VarViewId downDiagSum = _solver->makeIntVar(0, 0, n2 * n);
      propagation::VarViewId downDiagViol = _solver->makeIntVar(0, 0, n2 * n);
      std::vector<propagation::VarViewId> diag{};
      diag.reserve(n);
      for (int j = 0; j < n; ++j) {
        diag.push_back(square.at(j).at(j));
      }
      _solver->makeInvariant<propagation::Linear>(
          *_solver, downDiagSum, std::vector<propagation::VarViewId>(diag));
      _solver->makeViolationInvariant<propagation::Equal>(
          *_solver, downDiagViol, downDiagSum, magicSumVar);
      violations.push_back(downDiagViol);
    }

    {
      // upDiag
      propagation::VarViewId upDiagSum = _solver->makeIntVar(0, 0, n2 * n);
      propagation::VarViewId upDiagViol = _solver->makeIntVar(0, 0, n2 * n);
      std::vector<propagation::VarViewId> diag{};
      diag.reserve(n);
      for (int j = 0; j < n; ++j) {
        diag.push_back(square.at(n - j - 1).at(j));
      }
      _solver->makeInvariant<propagation::Linear>(
          *_solver, upDiagSum, std::vector<propagation::VarViewId>(diag));
      _solver->makeViolationInvariant<propagation::Equal>(
          *_solver, upDiagViol, upDiagSum, magicSumVar);
      violations.push_back(upDiagViol);
    }

    totalViolation = _solver->makeIntVar(0, 0, n2 * n2 * 2 + 2 * n2);
    _solver->makeInvariant<propagation::Linear>(
        *_solver, totalViolation,
        std::vector<propagation::VarViewId>(violations));
    _solver->close();
  }

  void TearDown() override {
    square.clear();
    flat.clear();
  }
};

/**
 *  Testing constructor
 */

[[maybe_unused]] std::string squareToString(MagicSquareTest& test) {
  std::string str = "\n";
  for (size_t i = 0; i < static_cast<size_t>(test.n); ++i) {
    for (size_t j = 0; j < static_cast<size_t>(test.n); ++j) {
      str +=
          std::to_string(test._solver->currentValue(test.square.at(i).at(j))) +
          " ";
    }
    str += "\n";
  }
  return str;
}

Int computeTotalViolaton(MagicSquareTest& test) {
  Int totalViol = 0;
  for (size_t i = 0; i < static_cast<size_t>(test.n); ++i) {
    Int rowSum = 0;
    Int colSum = 0;
    for (size_t j = 0; j < static_cast<size_t>(test.n); ++j) {
      rowSum += test._solver->currentValue(test.square.at(i).at(j));
      colSum += test._solver->currentValue(test.square.at(j).at(i));
    }
    totalViol += std::abs(rowSum - test.magicSum);
    totalViol += std::abs(colSum - test.magicSum);
  }
  Int downDiagSum = 0;
  Int upDiagSum = 0;
  for (size_t i = 0; i < static_cast<size_t>(test.n); ++i) {
    downDiagSum += test._solver->currentValue(test.square.at(i).at(i));
    upDiagSum +=
        test._solver->currentValue(test.square.at(test.n - i - 1).at(i));
  }

  totalViol += std::abs(downDiagSum - test.magicSum);
  totalViol += std::abs(upDiagSum - test.magicSum);
  return totalViol;
}

TEST_F(MagicSquareTest, Probing) {
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
      const Int oldI = _solver->committedValue(flat.at(i));
      const Int oldJ = _solver->committedValue(flat.at(j));
      _solver->beginMove();
      _solver->setValue(flat.at(i), oldJ);
      _solver->setValue(flat.at(j), oldI);
      _solver->endMove();

      _solver->beginProbe();
      _solver->query(totalViolation);
      _solver->endProbe();

      const Int totalViol = computeTotalViolaton(*this);

      EXPECT_EQ(totalViol, _solver->currentValue(totalViolation));
    }
  }

  std::vector<int> occurrences;
  occurrences.resize(flat.size(), 0);
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    occurrences.at(_solver->currentValue(flat.at(i)) - 1)++;
  }
  for (const int count : occurrences) {
    EXPECT_EQ(count, 1);
  }
}

TEST_F(MagicSquareTest, ProbeAndCommit) {
  for (size_t c = 0; c < 10; ++c) {
    for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
        const Int oldI = _solver->committedValue(flat.at(i));
        const Int oldJ = _solver->committedValue(flat.at(j));
        _solver->beginMove();
        _solver->setValue(flat.at(i), oldJ);
        _solver->setValue(flat.at(j), oldI);
        _solver->endMove();

        _solver->beginProbe();
        _solver->query(totalViolation);
        _solver->endProbe();

        const Int totalViol = computeTotalViolaton(*this);

        EXPECT_EQ(totalViol, _solver->currentValue(totalViolation));
      }
    }
    const Int i = distribution(gen);
    const Int j = distribution(gen);
    const Int oldI = _solver->committedValue(flat.at(i));
    const Int oldJ = _solver->committedValue(flat.at(j));
    // Perform random swap
    _solver->beginMove();
    _solver->setValue(flat.at(i), oldJ);
    _solver->setValue(flat.at(j), oldI);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(totalViolation);
    _solver->endProbe();
    const Int totalViol = computeTotalViolaton(*this);
    EXPECT_EQ(totalViol, _solver->currentValue(totalViolation));
  }

  std::vector<int> occurrences;
  occurrences.resize(flat.size(), 0);
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    occurrences.at(_solver->currentValue(flat.at(i)) - 1)++;
  }
  for (const int count : occurrences) {
    EXPECT_EQ(count, 1);
  }
}

}  // namespace atlantis::testing
