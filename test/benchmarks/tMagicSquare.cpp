#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include "../testHelper.hpp"
#include "propagation/violationInvariants/equal.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/solver.hpp"
#include "propagation/types.hpp"
#include "types.hpp"

namespace atlantis::testing {
class MagicSquareTest : public ::testing::Test {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::vector<std::vector<propagation::VarId>> square;
  std::vector<propagation::VarId> flat;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n;

  propagation::VarId totalViolation = propagation::NULL_ID;

  Int magicSum = 0;
  void SetUp() override {
    solver = std::make_unique<propagation::Solver>();
    n = 3;
    Int n2 = n * n;
    gen = std::mt19937(rd());

    magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<Int>{0, n2 - 1};

    solver->open();

    propagation::VarId magicSumVar =
        solver->makeIntVar(magicSum, magicSum, magicSum);

    for (int i = 0; i < n; ++i) {
      square.emplace_back();
      for (int j = 0; j < n; ++j) {
        auto var = solver->makeIntVar(i * n + j + 1, 1, n2);
        square.at(i).push_back(var);
        flat.push_back(var);
      }
    }

    std::vector<propagation::VarId> violations;

    // All different is implied by initial assignment + swap moves.
    // {
    //   propagation::VarId allDiffViol = solver->makeIntVar(0, 0, n2);
    //   violations.push_back(allDiffViol);
    //   solver->makeViolationInvariant<propagation::AllDifferent>(*solver, allDiffViol,
    //   flat);
    // }

    {
      // Row
      std::vector<Int> ones{};
      ones.assign(n, 1);
      for (int i = 0; i < n; ++i) {
        propagation::VarId rowSum = solver->makeIntVar(0, 0, n2 * n);
        propagation::VarId rowViol = solver->makeIntVar(0, 0, n2 * n);

        solver->makeInvariant<propagation::Linear>(*solver, rowSum,
                                                   square.at(i));
        solver->makeViolationInvariant<propagation::Equal>(*solver, rowViol, rowSum,
                                                   magicSumVar);
        violations.push_back(rowViol);
      }
    }

    {
      // Column
      std::vector<Int> ones{};
      ones.assign(n, 1);
      for (int i = 0; i < n; ++i) {
        propagation::VarId colSum = solver->makeIntVar(0, 0, n2 * n);
        propagation::VarId colViol = solver->makeIntVar(0, 0, n2 * n);
        std::vector<propagation::VarId> col{};
        col.reserve(n);
        for (int j = 0; j < n; ++j) {
          col.push_back(square.at(j).at(i));
        }
        solver->makeInvariant<propagation::Linear>(*solver, colSum, ones, col);
        solver->makeViolationInvariant<propagation::Equal>(*solver, colViol, colSum,
                                                   magicSumVar);
        violations.push_back(colViol);
      }
    }

    {
      // downDiag
      std::vector<Int> ones{};
      ones.assign(n, 1);
      propagation::VarId downDiagSum = solver->makeIntVar(0, 0, n2 * n);
      propagation::VarId downDiagViol = solver->makeIntVar(0, 0, n2 * n);
      std::vector<propagation::VarId> diag{};
      diag.reserve(n);
      for (int j = 0; j < n; ++j) {
        diag.push_back(square.at(j).at(j));
      }
      solver->makeInvariant<propagation::Linear>(*solver, downDiagSum, ones,
                                                 diag);
      solver->makeViolationInvariant<propagation::Equal>(*solver, downDiagViol,
                                                 downDiagSum, magicSumVar);
      violations.push_back(downDiagViol);
    }

    {
      // upDiag
      std::vector<Int> ones{};
      ones.assign(n, 1);
      propagation::VarId upDiagSum = solver->makeIntVar(0, 0, n2 * n);
      propagation::VarId upDiagViol = solver->makeIntVar(0, 0, n2 * n);
      std::vector<propagation::VarId> diag{};
      diag.reserve(n);
      for (int j = 0; j < n; ++j) {
        diag.push_back(square.at(n - j - 1).at(j));
      }
      solver->makeInvariant<propagation::Linear>(*solver, upDiagSum, ones,
                                                 diag);
      solver->makeViolationInvariant<propagation::Equal>(*solver, upDiagViol, upDiagSum,
                                                 magicSumVar);
      violations.push_back(upDiagViol);
    }

    std::vector<Int> ones{};
    ones.assign(violations.size(), 1);
    totalViolation = solver->makeIntVar(0, 0, n2 * n2 * 2 + 2 * n2);
    solver->makeInvariant<propagation::Linear>(*solver, totalViolation, ones,
                                               violations);
    solver->close();
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
          std::to_string(test.solver->currentValue(test.square.at(i).at(j))) +
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
      rowSum += test.solver->currentValue(test.square.at(i).at(j));
      colSum += test.solver->currentValue(test.square.at(j).at(i));
    }
    totalViol += std::abs(rowSum - test.magicSum);
    totalViol += std::abs(colSum - test.magicSum);
  }
  Int downDiagSum = 0;
  Int upDiagSum = 0;
  for (size_t i = 0; i < static_cast<size_t>(test.n); ++i) {
    downDiagSum += test.solver->currentValue(test.square.at(i).at(i));
    upDiagSum +=
        test.solver->currentValue(test.square.at(test.n - i - 1).at(i));
  }

  totalViol += std::abs(downDiagSum - test.magicSum);
  totalViol += std::abs(upDiagSum - test.magicSum);
  return totalViol;
}

TEST_F(MagicSquareTest, Probing) {
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
      const Int oldI = solver->committedValue(flat.at(i));
      const Int oldJ = solver->committedValue(flat.at(j));
      solver->beginMove();
      solver->setValue(flat.at(i), oldJ);
      solver->setValue(flat.at(j), oldI);
      solver->endMove();

      solver->beginProbe();
      solver->query(totalViolation);
      solver->endProbe();

      const Int totalViol = computeTotalViolaton(*this);
      logDebug(squareToString(*this));

      EXPECT_EQ(totalViol, solver->currentValue(totalViolation));
    }
  }

  std::vector<int> occurrences;
  occurrences.resize(flat.size(), 0);
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    occurrences.at(solver->currentValue(flat.at(i)) - 1)++;
  }
  for (const int count : occurrences) {
    EXPECT_EQ(count, 1);
  }
}

TEST_F(MagicSquareTest, ProbeAndCommit) {
  for (size_t c = 0; c < 10; ++c) {
    for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
        const Int oldI = solver->committedValue(flat.at(i));
        const Int oldJ = solver->committedValue(flat.at(j));
        solver->beginMove();
        solver->setValue(flat.at(i), oldJ);
        solver->setValue(flat.at(j), oldI);
        solver->endMove();

        solver->beginProbe();
        solver->query(totalViolation);
        solver->endProbe();

        const Int totalViol = computeTotalViolaton(*this);

        EXPECT_EQ(totalViol, solver->currentValue(totalViolation));
      }
    }
    const Int i = distribution(gen);
    const Int j = distribution(gen);
    const Int oldI = solver->committedValue(flat.at(i));
    const Int oldJ = solver->committedValue(flat.at(j));
    // Perform random swap
    solver->beginMove();
    solver->setValue(flat.at(i), oldJ);
    solver->setValue(flat.at(j), oldI);
    solver->endMove();

    solver->beginProbe();
    solver->query(totalViolation);
    solver->endProbe();
    const Int totalViol = computeTotalViolaton(*this);
    EXPECT_EQ(totalViol, solver->currentValue(totalViolation));
  }

  std::vector<int> occurrences;
  occurrences.resize(flat.size(), 0);
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    occurrences.at(solver->currentValue(flat.at(i)) - 1)++;
  }
  for (const int count : occurrences) {
    EXPECT_EQ(count, 1);
  }
}

}  // namespace atlantis::testing