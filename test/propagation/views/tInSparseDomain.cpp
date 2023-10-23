#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../../testHelper.hpp"
#include "propagation/propagationEngine.hpp"
#include "propagation/views/inSparseDomain.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class InSparseDomainTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  std::mt19937 gen;
  std::default_random_engine rng;

 public:
  Int computeViolation(Timestamp ts, VarId var,
                       const std::vector<DomainEntry>& domain) {
    return computeViolation(engine->value(ts, var), domain);
  }
  Int computeViolation(Int val, const std::vector<DomainEntry>& domain) {
    Int viol = std::numeric_limits<Int>::max();
    for (const auto& [lb, ub] : domain) {
      if (lb <= val && val <= ub) {
        viol = 0;
        break;
      } else if (val < lb) {
        viol = std::min(viol, lb - val);
      } else if (ub < val) {
        viol = std::min(viol, val - ub);
      }
    }
    return viol;
  }
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
  }
};

TEST_F(InSparseDomainTest, Bounds) {
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  engine->open();
  const VarId x = engine->makeIntVar(domainVec.front().lowerBound,
                                     domainVec.front().lowerBound,
                                     domainVec.front().upperBound);

  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    for (const auto& [xLb, xUb] : domainVec) {
      EXPECT_TRUE(xLb <= xUb);
      if (!engine->isOpen()) {
        engine->open();
      }
      engine->updateBounds(x, xLb, xUb, false);
      const VarId violationId = engine->makeIntView<InSparseDomain>(
          *engine, x, std::vector<DomainEntry>(dom));
      engine->close();

      std::vector<Int> violations;
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        engine->setValue(engine->currentTimestamp(), x, xVal);
        violations.emplace_back(
            engine->value(engine->currentTimestamp(), violationId));
      }
      const auto& [minViol, maxViol] =
          std::minmax_element(violations.begin(), violations.end());
      ASSERT_GE(*minViol, engine->lowerBound(violationId));
      ASSERT_LE(*maxViol, engine->upperBound(violationId));
    }
  }
}

TEST_F(InSparseDomainTest, Value) {
  const Int margin = 20;
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    if (!engine->isOpen()) {
      engine->open();
    }
    const Int lb = domainVec.front().lowerBound - margin;
    const Int ub = domainVec.back().upperBound + margin;
    const VarId x = engine->makeIntVar(lb, lb, ub);
    const VarId violationId = engine->makeIntView<InSparseDomain>(
        *engine, x, std::vector<DomainEntry>(dom));
    engine->close();
    for (Int val = lb; val <= ub; ++val) {
      engine->setValue(engine->currentTimestamp(), x, val);
      EXPECT_EQ(computeViolation(val, dom),
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(InSparseDomainTest, CommittedValue) {
  const Int margin = 20;
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    if (!engine->isOpen()) {
      engine->open();
    }
    const Int lb = domainVec.front().lowerBound - margin;
    const Int ub = domainVec.back().upperBound + margin;

    std::vector<Int> values(ub - lb + 1);
    std::iota(values.begin(), values.end(), lb);
    EXPECT_EQ(values.front(), lb);
    EXPECT_EQ(values.back(), ub);

    std::shuffle(values.begin(), values.end(), rng);

    const VarId x = engine->makeIntVar(lb, lb, ub);
    const VarId violationId = engine->makeIntView<InSparseDomain>(
        *engine, x, std::vector<DomainEntry>(dom));
    engine->close();

    Int committedValue = engine->committedValue(x);

    for (size_t i = 0; i < values.size(); ++i) {
      Timestamp ts = engine->currentTimestamp() + Timestamp(1 + i);
      ASSERT_EQ(engine->committedValue(x), committedValue);

      engine->setValue(ts, x, values[i]);

      const Int expectedViol = computeViolation(values[i], dom);

      ASSERT_EQ(expectedViol, engine->value(ts, violationId));

      engine->commitIf(ts, x);
      committedValue = engine->value(ts, x);

      ASSERT_EQ(expectedViol, engine->value(ts + 1, violationId));
    }
  }
}
}  // namespace atlantis::testing