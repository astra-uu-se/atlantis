#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_lin_le.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using ::rc::gen::inRange;
using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_lin_leTest : public FznTestBase {
 public:
  std::vector<std::string> inputIdentifiers{};
  std::vector<Int> coeffs{};
  std::vector<std::pair<Int, Int>> varBounds{};
  Int bound = 10;

  Int isViolated(const std::vector<Int>& inputVals) {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      sum += coeffs.at(i) * inputVals.at(i);
    }
    return sum > bound;
  }

  void generate() {
    inputIdentifiers.reserve(coeffs.size());
    std::vector<Arg> args;
    args.reserve(2);

    for (size_t i = 0; i < coeffs.size(); ++i) {
      inputIdentifiers.emplace_back("i_" + std::to_string(i));
      _model->addVar(std::make_shared<IntVar>(
          varBounds[i].first, varBounds[i].second, inputIdentifiers.back()));
    }

    auto coeffsArg = std::make_shared<IntVarArray>("coeffs");
    auto inputsArg = std::make_shared<IntVarArray>("inputs");
    for (size_t i = 0; i < coeffs.size(); ++i) {
      coeffsArg->append(coeffs[i]);
      inputsArg->append(std::get<std::shared_ptr<IntVar>>(
          _model->var(inputIdentifiers.at(i))));
    }
    args.emplace_back(coeffsArg);
    args.emplace_back(inputsArg);
    args.emplace_back(IntArg(bound));

    _model->addConstraint(Constraint(constraintIdentifier, std::move(args)));
  }

  void generateSimple() {
    const Int numInputs = 4;
    coeffs.reserve(numInputs);
    varBounds.reserve(numInputs);
    for (Int i = 0; i < numInputs; ++i) {
      coeffs.emplace_back((i + 1) * (i % 2 == 0 ? 1 : -1));
      varBounds.emplace_back(-2, 2);
    }
    generate();
  }

  void SetUp() override {
    FznTestBase::SetUp();
    constraintIdentifier = "int_lin_le";
  }
};

TEST_F(int_lin_leTest, construction) {
  generateSimple();
  EXPECT_EQ(_model->constraints().size(), 1);
  EXPECT_TRUE(int_lin_le(*_invariantGraph, _model->constraints().front()));
  for (const auto& identifier : inputIdentifiers) {
    EXPECT_TRUE(_invariantGraph->containsVarNode(identifier));
    EXPECT_NE(_invariantGraph->varNodeId(identifier), NULL_NODE_ID);
  }
}

TEST_F(int_lin_leTest, propagation) {
  generateSimple();
  int_lin_le(*_invariantGraph, _model->constraints().front());
  _invariantGraph->apply(*_solver);

  std::vector<Int> inputVals = makeInputVals(inputIdentifiers);

  while (increaseNextVal(inputIdentifiers, inputVals)) {
    _solver->beginMove();
    setVarVals(inputIdentifiers, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(totalViolationVarId());
    _solver->endProbe();

    const bool actual = violation() > 0;
    const bool expected = isViolated(inputVals);

    EXPECT_EQ(actual, expected);
  }
}

static std::pair<Int, Int> computeLinearBounds(
    const std::vector<Int>& coeffs,
    const std::vector<std::pair<Int, Int>>& bounds) {
  Int sumLb = 0;
  Int sumUb = 0;
  for (size_t i = 0; i < coeffs.size(); ++i) {
    const Int lb =
        std::min(coeffs[i] * bounds[i].first, coeffs[i] * bounds[i].second);
    const Int ub =
        std::max(coeffs[i] * bounds[i].first, coeffs[i] * bounds[i].second);
    assert((lb < 0 && sumLb + lb <= sumLb) || (lb >= 0 && sumLb + lb >= sumLb));
    assert((ub < 0 && sumUb + ub <= sumUb) || (ub >= 0 && sumUb + ub >= sumUb));
    sumLb += lb;
    sumUb += ub;
    assert(sumLb <= sumUb);
  }
  return {sumLb, sumUb};
}

RC_GTEST_FIXTURE_PROP(int_lin_leTest, rapidcheck, (Int bMax)) {
  const size_t numInputs = *rc::gen::inRange<size_t>(0, 256);
  bound = bMax;

  coeffs.clear();
  coeffs.reserve(numInputs);

  const Int minLb =
      std::numeric_limits<Int>::min() / (static_cast<Int>(numInputs) + 1);
  const Int maxUb =
      std::numeric_limits<Int>::max() / (static_cast<Int>(numInputs) + 1);

  for (size_t i = 0; i < numInputs; ++i) {
    coeffs.emplace_back(*rc::gen::inRange<Int>(minLb, maxUb));
  }

  varBounds.reserve(numInputs);
  for (size_t i = 0; i < numInputs; ++i) {
    Int lb, ub;
    if (coeffs[i] == 0) {
      lb = 0;
      ub = 0;
    } else {
      lb = std::min<Int>(minLb / coeffs[i], maxUb / coeffs[i]);
      ub = std::max<Int>(minLb / coeffs[i], maxUb / coeffs[i]);
      RC_ASSERT(lb <= ub);
    }
    if (lb == ub) {
      varBounds.emplace_back(lb, ub);
    } else {
      Int b1 = *inRange<Int>(lb, ub);
      Int b2 = *inRange<Int>(lb, ub);
      varBounds.emplace_back(std::min(b1, b2), std::max(b1, b2));
    }
  }

  for (size_t i = 0; i < numInputs; ++i) {
    RC_ASSERT(varBounds[i].first <= varBounds[i].second);
  }

  const auto [sumLb, sumUb] = computeLinearBounds(coeffs, varBounds);

  RC_ASSERT(sumLb <= sumUb);

  generate();

  for (size_t i = 0; i < numInputs; ++i) {
    RC_ASSERT(_model->hasVar(inputIdentifiers[i]));
    RC_ASSERT(std::holds_alternative<std::shared_ptr<fznparser::IntVar>>(
        _model->var(inputIdentifiers[i])));
    auto var = std::get<std::shared_ptr<fznparser::IntVar>>(
        _model->var(inputIdentifiers[i]));
    RC_ASSERT(var->lowerBound() == varBounds[i].first);
    RC_ASSERT(var->upperBound() == varBounds[i].second);
  }

  if (sumLb > bound) {
    RC_ASSERT_THROWS_AS(
        int_lin_le(*_invariantGraph, _model->constraints().back()),
        FznArgumentException);
    RC_SUCCEED("Never holds");
  }

  RC_ASSERT(int_lin_le(*_invariantGraph, _model->constraints().back()));

  _invariantGraph->apply(*_solver);

  if (sumUb <= bound) {
    const propagation::VarId viol = totalViolationVarId();
    RC_ASSERT(viol == propagation::NULL_ID || (_solver->upperBound(viol) == 0));
    return;
  }

  for (size_t i = 0; i < numInputs; ++i) {
    RC_ASSERT_FALSE(_invariantGraph->varNodeId(inputIdentifiers[i]) ==
                    NULL_NODE_ID);
  }

  RC_ASSERT_FALSE(totalViolationVarId() == propagation::NULL_ID);
  Int sum = 0;
  _solver->beginMove();
  for (size_t i = 0; i < numInputs; ++i) {
    const Int val = *inRange(varBounds[i].first, varBounds[i].second);
    sum += coeffs[i] * val;
    _solver->setValue(varId(inputIdentifiers[i]), val);
  }
  _solver->endMove();
  _solver->beginProbe();
  _solver->query(_invariantGraph->totalViolationVarId());
  _solver->endProbe();
  const Int actual = violation() > 0;
  const Int expected = sum > bound;
  RC_ASSERT(actual == expected);
}

}  // namespace atlantis::testing
