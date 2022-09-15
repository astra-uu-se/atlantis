#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include "../testHelper.hpp"
#include "constraints/equal.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/linear.hpp"

namespace {
class MagicSquareTest : public ::testing::Test {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<std::vector<VarId>> square;
  std::vector<VarId> flat;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n;

  VarId totalViolation = NULL_ID;

  Int magicSum = 0;
  void SetUp() override {
    engine = std::make_unique<PropagationEngine>();
    n = 3;
    Int n2 = n * n;
    gen = std::mt19937(rd());

    magicSum = (n * n * (n * n + 1) / 2) / n;

    distribution = std::uniform_int_distribution<Int>{0, n2 - 1};

    engine->open();

    VarId magicSumVar = engine->makeIntVar(magicSum, magicSum, magicSum);

    for (int i = 0; i < n; ++i) {
      square.emplace_back();
      for (int j = 0; j < n; ++j) {
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
      for (int i = 0; i < n; ++i) {
        VarId rowSum = engine->makeIntVar(0, 0, n2 * n);
        VarId rowViol = engine->makeIntVar(0, 0, n2 * n);

        engine->makeInvariant<Linear>(rowSum, square.at(i));
        engine->makeConstraint<Equal>(rowViol, rowSum, magicSumVar);
        violations.push_back(rowViol);
      }
    }

    {
      // Column
      std::vector<Int> ones{};
      ones.assign(n, 1);
      for (int i = 0; i < n; ++i) {
        VarId colSum = engine->makeIntVar(0, 0, n2 * n);
        VarId colViol = engine->makeIntVar(0, 0, n2 * n);
        std::vector<VarId> col{};
        col.reserve(n);
        for (int j = 0; j < n; ++j) {
          col.push_back(square.at(j).at(i));
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
      VarId downDiagSum = engine->makeIntVar(0, 0, n2 * n);
      VarId downDiagViol = engine->makeIntVar(0, 0, n2 * n);
      std::vector<VarId> diag{};
      diag.reserve(n);
      for (int j = 0; j < n; ++j) {
        diag.push_back(square.at(j).at(j));
      }
      engine->makeInvariant<Linear>(downDiagSum, ones, diag);
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
      diag.reserve(n);
      for (int j = 0; j < n; ++j) {
        diag.push_back(square.at(n - j - 1).at(j));
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
          std::to_string(test.engine->currentValue(test.square.at(i).at(j))) +
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
      rowSum += test.engine->currentValue(test.square.at(i).at(j));
      colSum += test.engine->currentValue(test.square.at(j).at(i));
    }
    totalViol += std::abs(rowSum - test.magicSum);
    totalViol += std::abs(colSum - test.magicSum);
  }
  Int downDiagSum = 0;
  Int upDiagSum = 0;
  for (size_t i = 0; i < static_cast<size_t>(test.n); ++i) {
    downDiagSum += test.engine->currentValue(test.square.at(i).at(i));
    upDiagSum +=
        test.engine->currentValue(test.square.at(test.n - i - 1).at(i));
  }

  totalViol += std::abs(downDiagSum - test.magicSum);
  totalViol += std::abs(upDiagSum - test.magicSum);
  return totalViol;
}

TEST_F(MagicSquareTest, Probing) {
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
      const Int oldI = engine->committedValue(flat.at(i));
      const Int oldJ = engine->committedValue(flat.at(j));
      engine->beginMove();
      engine->setValue(flat.at(i), oldJ);
      engine->setValue(flat.at(j), oldI);
      engine->endMove();

      engine->beginProbe();
      engine->query(totalViolation);
      engine->endProbe();

      const Int totalViol = computeTotalViolaton(*this);
      logDebug(squareToString(*this));

      EXPECT_EQ(totalViol, engine->currentValue(totalViolation));
    }
  }

  std::vector<int> occurrences;
  occurrences.resize(flat.size(), 0);
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    occurrences.at(engine->currentValue(flat.at(i)) - 1)++;
  }
  for (const int count : occurrences) {
    EXPECT_EQ(count, 1);
  }
}

TEST_F(MagicSquareTest, ProbeAndCommit) {
  for (size_t c = 0; c < 10; ++c) {
    for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n * n); ++j) {
        const Int oldI = engine->committedValue(flat.at(i));
        const Int oldJ = engine->committedValue(flat.at(j));
        engine->beginMove();
        engine->setValue(flat.at(i), oldJ);
        engine->setValue(flat.at(j), oldI);
        engine->endMove();

        engine->beginProbe();
        engine->query(totalViolation);
        engine->endProbe();

        const Int totalViol = computeTotalViolaton(*this);

        EXPECT_EQ(totalViol, engine->currentValue(totalViolation));
      }
    }
    const Int i = distribution(gen);
    const Int j = distribution(gen);
    const Int oldI = engine->committedValue(flat.at(i));
    const Int oldJ = engine->committedValue(flat.at(j));
    // Perform random swap
    engine->beginMove();
    engine->setValue(flat.at(i), oldJ);
    engine->setValue(flat.at(j), oldI);
    engine->endMove();

    engine->beginProbe();
    engine->query(totalViolation);
    engine->endProbe();
    const Int totalViol = computeTotalViolaton(*this);
    EXPECT_EQ(totalViol, engine->currentValue(totalViolation));
  }

  std::vector<int> occurrences;
  occurrences.resize(flat.size(), 0);
  for (size_t i = 0; i < static_cast<size_t>(n * n); ++i) {
    occurrences.at(engine->currentValue(flat.at(i)) - 1)++;
  }
  for (const int count : occurrences) {
    EXPECT_EQ(count, 1);
  }
}

}  // namespace
