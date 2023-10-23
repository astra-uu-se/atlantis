#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "propagation/constraints/lessEqual.hpp"
#include "propagation/invariants/elementVar.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/propagationEngine.hpp"
#include "propagation/types.hpp"
#include "propagation/views/elementConst.hpp"
#include "types.hpp"

namespace atlantis::testing {
class TSPTWTest : public ::testing::Test {
 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  std::vector<propagation::VarId> pred;
  std::vector<propagation::VarId> timeToPrev;
  std::vector<propagation::VarId> arrivalPrev;
  std::vector<propagation::VarId> arrivalTime;
  std::vector<std::vector<Int>> dist;
  propagation::VarId totalDist;

  std::random_device rd;

  Int n;
  const int MAX_TIME = 100000;

  std::vector<propagation::VarId> violation;
  propagation::VarId totalViolation;

  void SetUp() override {
    engine = std::make_unique<propagation::PropagationEngine>();
    n = 30;

    logInfo(n);
    engine->open();

    for (int i = 0; i < n; ++i) {
      dist.emplace_back(std::vector<Int>());
      for (int j = 0; j < n; ++j) {
        dist[i].push_back(1);
      }
    }

    for (int i = 1; i <= n; ++i) {
      const Int initVal = 1 + (i % n);
      pred.emplace_back(engine->makeIntVar(initVal, 1, n));
      timeToPrev.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      arrivalTime.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      arrivalPrev.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      violation.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // timeToPrev[i] = dist[i][pred[i]]
      timeToPrev[i] = engine->makeIntView<propagation::ElementConst>(
          *engine, pred[i], dist[i]);
      // arrivalPrev[i] = arrivalTime[pred[i]]
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // arrivalPrev[i] = arrivalTime[pred[i]]
      engine->makeInvariant<propagation::ElementVar>(*engine, arrivalPrev[i],
                                                     pred[i], arrivalTime);
      // arrivalTime[i] = arrivalPrev[i] + timeToPrev[i]
      engine->makeInvariant<propagation::Linear>(
          *engine, arrivalTime[i],
          std::vector<propagation::VarId>({arrivalPrev[i], timeToPrev[i]}));
    }

    // totalDist = sum(timeToPrev)
    totalDist = engine->makeIntVar(0, 0, MAX_TIME);
    engine->makeInvariant<propagation::Linear>(*engine, totalDist, timeToPrev);

    propagation::VarId leqConst = engine->makeIntVar(100, 100, 100);
    for (int i = 0; i < n; ++i) {
      engine->makeConstraint<propagation::LessEqual>(*engine, violation[i],
                                                     arrivalTime[i], leqConst);
    }

    totalViolation = engine->makeIntVar(0, 0, MAX_TIME * n);
    engine->makeInvariant<propagation::Linear>(*engine, totalViolation,
                                               violation);

    engine->close();
    for (const propagation::VarId p : pred) {
      assert(engine->lowerBound(p) == 1);
      assert(engine->upperBound(p) == n);
      assert(1 <= engine->committedValue(p) && engine->committedValue(p) <= n);
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
      if (i == j || engine->committedValue(pred[i]) == j + 1) {
        continue;
      }
      engine->beginMove();
      engine->setValue(
          pred[i],
          engine->committedValue(pred.at(engine->committedValue(pred[i]) - 1)));
      engine->setValue(pred[j], engine->committedValue(pred[i]));
      engine->setValue(pred.at(engine->committedValue(pred[i]) - 1),
                       engine->committedValue(pred[j]));
      engine->endMove();

      engine->beginProbe();
      engine->query(totalDist);
      engine->query(totalViolation);
      engine->endProbe();
    }
  }
}

}  // namespace atlantis::testing
