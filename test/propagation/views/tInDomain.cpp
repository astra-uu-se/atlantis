#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../../testHelper.hpp"
#include "propagation/solver.hpp"
#include "propagation/views/inDomain.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class InDomainTest : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;
  std::mt19937 gen;
  std::default_random_engine rng;

 public:
  Int computeViolation(Timestamp ts, VarId var,
                       const std::vector<DomainEntry>& domain) {
    return computeViolation(solver->value(ts, var), domain);
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
    solver = std::make_unique<Solver>();
  }
};

TEST_F(InDomainTest, Bounds) {
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  solver->open();
  const VarId x = solver->makeIntVar(domainVec.front().lowerBound,
                                     domainVec.front().lowerBound,
                                     domainVec.front().upperBound);

  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    for (const auto& [xLb, xUb] : domainVec) {
      EXPECT_TRUE(xLb <= xUb);
      if (!solver->isOpen()) {
        solver->open();
      }
      solver->updateBounds(x, xLb, xUb, false);
      const VarId violationId = solver->makeIntView<InDomain>(
          *solver, x, std::vector<DomainEntry>(dom));
      solver->close();

      std::vector<Int> violations;
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        solver->setValue(solver->currentTimestamp(), x, xVal);
        violations.emplace_back(
            solver->value(solver->currentTimestamp(), violationId));
      }
      const auto& [minViol, maxViol] =
          std::minmax_element(violations.begin(), violations.end());
      ASSERT_EQ(*minViol, solver->lowerBound(violationId));
      ASSERT_LE(*maxViol, solver->upperBound(violationId));
    }
  }
}

TEST_F(InDomainTest, Value) {
  const Int margin = 20;
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    if (!solver->isOpen()) {
      solver->open();
    }
    const Int lb = domainVec.front().lowerBound - margin;
    const Int ub = domainVec.back().upperBound + margin;
    const VarId x = solver->makeIntVar(lb, lb, ub);
    const VarId violationId = solver->makeIntView<InDomain>(
        *solver, x, std::vector<DomainEntry>(dom));
    solver->close();
    for (Int val = lb; val <= ub; ++val) {
      solver->setValue(solver->currentTimestamp(), x, val);
      EXPECT_EQ(computeViolation(val, dom),
                solver->value(solver->currentTimestamp(), violationId));
    }
  }
}

TEST_F(InDomainTest, CommittedValue) {
  const Int margin = 20;
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    if (!solver->isOpen()) {
      solver->open();
    }
    const Int lb = domainVec.front().lowerBound - margin;
    const Int ub = domainVec.back().upperBound + margin;

    std::vector<Int> values(ub - lb + 1);
    std::iota(values.begin(), values.end(), lb);
    EXPECT_EQ(values.front(), lb);
    EXPECT_EQ(values.back(), ub);

    std::shuffle(values.begin(), values.end(), rng);

    const VarId x = solver->makeIntVar(lb, lb, ub);
    const VarId violationId = solver->makeIntView<InDomain>(
        *solver, x, std::vector<DomainEntry>(dom));
    solver->close();

    Int committedValue = solver->committedValue(x);

    for (size_t i = 0; i < values.size(); ++i) {
      Timestamp ts = solver->currentTimestamp() + Timestamp(1 + i);
      ASSERT_EQ(solver->committedValue(x), committedValue);

      solver->setValue(ts, x, values[i]);

      const Int expectedViol = computeViolation(values[i], dom);

      ASSERT_EQ(expectedViol, solver->value(ts, violationId));

      solver->commitIf(ts, x);
      committedValue = solver->value(ts, x);

      ASSERT_EQ(expectedViol, solver->value(ts + 1, violationId));
    }
  }
}
}  // namespace atlantis::testing