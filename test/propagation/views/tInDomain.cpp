#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../../testHelper.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/inDomain.hpp"
#include "atlantis/types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class InDomainTest : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;
  std::mt19937 gen;
  std::default_random_engine rng;

 public:
  Int computeOutput(Timestamp ts, VarViewId var,
                    const std::vector<DomainEntry>& domain) {
    return computeOutput(_solver->value(ts, var), domain);
  }
  static Int computeOutput(Int val, const std::vector<DomainEntry>& domain) {
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
    _solver = std::make_shared<Solver>();
  }
};

TEST_F(InDomainTest, Bounds) {
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  _solver->open();
  const VarViewId x = _solver->makeIntVar(domainVec.front().lowerBound,
                                          domainVec.front().lowerBound,
                                          domainVec.front().upperBound);

  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.empty()) {
      continue;
    }
    for (const auto& [xLb, xUb] : domainVec) {
      EXPECT_LE(xLb, xUb);
      if (!_solver->isOpen()) {
        _solver->open();
      }
      _solver->updateBounds(VarId(x), xLb, xUb, false);
      const VarViewId violationId = _solver->makeIntView<InDomain>(
          *_solver, x, std::vector<DomainEntry>(dom));
      _solver->close();

      std::vector<Int> violations;
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        _solver->setValue(_solver->currentTimestamp(), x, xVal);
        violations.emplace_back(_solver->currentValue(violationId));
      }
      const auto& [minViol, maxViol] =
          std::minmax_element(violations.begin(), violations.end());
      ASSERT_EQ(*minViol, _solver->lowerBound(violationId));
      ASSERT_LE(*maxViol, _solver->upperBound(violationId));
    }
  }
}

TEST_F(InDomainTest, Value) {
  const Int margin = 20;
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.empty()) {
      continue;
    }
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const Int lb = domainVec.front().lowerBound - margin;
    const Int ub = domainVec.back().upperBound + margin;
    const VarViewId x = _solver->makeIntVar(lb, lb, ub);
    const VarViewId violationId = _solver->makeIntView<InDomain>(
        *_solver, x, std::vector<DomainEntry>(dom));
    _solver->close();
    for (Int val = lb; val <= ub; ++val) {
      _solver->setValue(_solver->currentTimestamp(), x, val);
      EXPECT_EQ(computeOutput(val, dom), _solver->currentValue(violationId));
    }
  }
}

TEST_F(InDomainTest, CommittedValue) {
  const Int margin = 20;
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.empty()) {
      continue;
    }
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const Int lb = domainVec.front().lowerBound - margin;
    const Int ub = domainVec.back().upperBound + margin;

    std::vector<Int> values(ub - lb + 1);
    std::iota(values.begin(), values.end(), lb);
    EXPECT_EQ(values.front(), lb);
    EXPECT_EQ(values.back(), ub);

    std::shuffle(values.begin(), values.end(), rng);

    const VarViewId x = _solver->makeIntVar(lb, lb, ub);
    const VarViewId violationId = _solver->makeIntView<InDomain>(
        *_solver, x, std::vector<DomainEntry>(dom));
    _solver->close();

    Int committedValue = _solver->committedValue(x);

    for (size_t i = 0; i < values.size(); ++i) {
      Timestamp ts = _solver->currentTimestamp() + Timestamp(1 + i);
      ASSERT_EQ(_solver->committedValue(x), committedValue);

      _solver->setValue(ts, x, values[i]);

      const Int expectedViol = computeOutput(values[i], dom);

      ASSERT_EQ(expectedViol, _solver->value(ts, violationId));

      _solver->commitIf(ts, VarId(x));
      committedValue = _solver->value(ts, x);

      ASSERT_EQ(expectedViol, _solver->value(ts + 1, violationId));
    }
  }
}
}  // namespace atlantis::testing
