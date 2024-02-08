#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "propagation/violationInvariants/lessEqual.hpp"
#include "propagation/invariants/elementVar.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/solver.hpp"
#include "propagation/types.hpp"
#include "propagation/views/elementConst.hpp"
#include "types.hpp"

namespace atlantis::testing {
class TSPTWTest : public ::testing::Test {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::vector<propagation::VarId> pred;
  std::vector<propagation::VarId> timeToPrev;
  std::vector<propagation::VarId> arrivalPrev;
  std::vector<propagation::VarId> arrivalTime;
  std::vector<std::vector<Int>> dist;
  propagation::VarId totalDist;

  std::random_device rd;

  Int n{30};
  const int MAX_TIME = 100000;

  std::vector<propagation::VarId> violation;
  propagation::VarId totalViolation;

  void SetUp() override {
    solver = std::make_unique<propagation::Solver>();
    n = 30;

    logInfo(n);
    solver->open();

    for (int i = 0; i < n; ++i) {
      dist.emplace_back();
      for (int j = 0; j < n; ++j) {
        dist[i].push_back(1);
      }
    }

    for (int i = 1; i <= n; ++i) {
      const Int initVal = 1 + (i % n);
      pred.emplace_back(solver->makeIntVar(initVal, 1, n));
      timeToPrev.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
      arrivalTime.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
      arrivalPrev.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
      violation.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // timeToPrev[i] = dist[i][pred[i]]
      timeToPrev[i] = solver->makeIntView<propagation::ElementConst>(
          *solver, pred[i], std::vector<Int>(dist[i]));
      // arrivalPrev[i] = arrivalTime[pred[i]]
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // arrivalPrev[i] = arrivalTime[pred[i]]
      solver->makeInvariant<propagation::ElementVar>(*solver, arrivalPrev[i],
                                                     pred[i], std::vector<propagation::VarId>(arrivalTime));
      // arrivalTime[i] = arrivalPrev[i] + timeToPrev[i]
      solver->makeInvariant<propagation::Linear>(
          *solver, arrivalTime[i],
          std::vector<propagation::VarId>({arrivalPrev[i], timeToPrev[i]}));
    }

    // totalDist = sum(timeToPrev)
    totalDist = solver->makeIntVar(0, 0, MAX_TIME);
    solver->makeInvariant<propagation::Linear>(*solver, totalDist, std::vector<propagation::VarId>(timeToPrev));

    propagation::VarId leqConst = solver->makeIntVar(100, 100, 100);
    for (int i = 0; i < n; ++i) {
      solver->makeViolationInvariant<propagation::LessEqual>(*solver, violation[i],
                                                     arrivalTime[i], leqConst);
    }

    totalViolation = solver->makeIntVar(0, 0, MAX_TIME * n);
    solver->makeInvariant<propagation::Linear>(*solver, totalViolation,
                                               std::vector<propagation::VarId>(violation));

    solver->close();
    for (const propagation::VarId& p : pred) {
      assert(solver->lowerBound(p) == 1);
      assert(solver->upperBound(p) == n);
      assert(1 <= solver->committedValue(p) && solver->committedValue(p) <= n);
    }
  }

  void TearDown() override {
    pred.clear();
    timeToPrev.clear();
    arrivalPrev.clear();
    arrivalTime.clear();
    dist.clear();
    violation.clear();
  }
};

/**
 *  Testing constructor
 */

TEST_F(TSPTWTest, Probing) {
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      if (i == j || solver->committedValue(pred[i]) == j + 1) {
        continue;
      }
      solver->beginMove();
      solver->setValue(
          pred[i],
          solver->committedValue(pred.at(solver->committedValue(pred[i]) - 1)));
      solver->setValue(pred[j], solver->committedValue(pred[i]));
      solver->setValue(pred.at(solver->committedValue(pred[i]) - 1),
                       solver->committedValue(pred[j]));
      solver->endMove();

      solver->beginProbe();
      solver->query(totalDist);
      solver->query(totalViolation);
      solver->endProbe();
    }
  }
}

}  // namespace atlantis::testing
