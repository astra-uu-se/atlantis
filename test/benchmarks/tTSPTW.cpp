#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/views/elementConst.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "atlantis/types.hpp"

namespace atlantis::testing {
class TSPTWTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
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
    _solver = std::make_shared<propagation::Solver>();
    n = 30;

    logInfo(n);
    _solver->open();

    for (int i = 0; i < n; ++i) {
      dist.emplace_back();
      for (int j = 0; j < n; ++j) {
        dist[i].push_back(1);
      }
    }

    for (int i = 1; i <= n; ++i) {
      const Int initVal = 1 + (i % n);
      pred.emplace_back(_solver->makeIntVar(initVal, 1, n));
      timeToPrev.emplace_back(_solver->makeIntVar(0, 0, MAX_TIME));
      arrivalTime.emplace_back(_solver->makeIntVar(0, 0, MAX_TIME));
      arrivalPrev.emplace_back(_solver->makeIntVar(0, 0, MAX_TIME));
      violation.emplace_back(_solver->makeIntVar(0, 0, MAX_TIME));
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // timeToPrev[i] = dist[i][pred[i]]
      timeToPrev[i] = _solver->makeIntView<propagation::ElementConst>(
          *_solver, pred[i], std::vector<Int>(dist[i]));
      // arrivalPrev[i] = arrivalTime[pred[i]]
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // arrivalPrev[i] = arrivalTime[pred[i]]
      _solver->makeInvariant<propagation::ElementVar>(
          *_solver, arrivalPrev[i], pred[i],
          std::vector<propagation::VarId>(arrivalTime));
      // arrivalTime[i] = arrivalPrev[i] + timeToPrev[i]
      _solver->makeInvariant<propagation::Linear>(
          *_solver, arrivalTime[i],
          std::vector<propagation::VarId>({arrivalPrev[i], timeToPrev[i]}));
    }

    // totalDist = sum(timeToPrev)
    totalDist = _solver->makeIntVar(0, 0, MAX_TIME);
    _solver->makeInvariant<propagation::Linear>(
        *_solver, totalDist, std::vector<propagation::VarId>(timeToPrev));

    propagation::VarId leqConst = _solver->makeIntVar(100, 100, 100);
    for (int i = 0; i < n; ++i) {
      _solver->makeViolationInvariant<propagation::LessEqual>(
          *_solver, violation[i], arrivalTime[i], leqConst);
    }

    totalViolation = _solver->makeIntVar(0, 0, MAX_TIME * n);
    _solver->makeInvariant<propagation::Linear>(
        *_solver, totalViolation, std::vector<propagation::VarId>(violation));

    _solver->close();
    for (const propagation::VarId& p : pred) {
      assert(_solver->lowerBound(p) == 1);
      assert(_solver->upperBound(p) == n);
      assert(1 <= _solver->committedValue(p) &&
             _solver->committedValue(p) <= n);
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
      if (i == j || _solver->committedValue(pred[i]) == j + 1) {
        continue;
      }
      _solver->beginMove();
      _solver->setValue(pred[i], _solver->committedValue(pred.at(
                                     _solver->committedValue(pred[i]) - 1)));
      _solver->setValue(pred[j], _solver->committedValue(pred[i]));
      _solver->setValue(pred.at(_solver->committedValue(pred[i]) - 1),
                        _solver->committedValue(pred[j]));
      _solver->endMove();

      _solver->beginProbe();
      _solver->query(totalDist);
      _solver->query(totalViolation);
      _solver->endProbe();
    }
  }
}

}  // namespace atlantis::testing
