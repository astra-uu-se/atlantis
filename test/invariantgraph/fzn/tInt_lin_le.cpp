#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <deque>
#include <random>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "invariantgraph/fzn/int_lin_le.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_lin_leTest : public FznTestBase {
 public:
  std::vector<std::string> inputIdentifiers{};
  std::vector<Int> coeffs{};
  std::vector<std::pair<Int, Int>> varBounds{};
  Int bound = 20;

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
      _model->addVar(std::move(IntVar(varBounds[i].first, varBounds[i].second,
                                      inputIdentifiers.back())));
    }

    IntVarArray coeffsArg("coeffs");
    IntVarArray inputsArg("inputs");
    for (size_t i = 0; i < coeffs.size(); ++i) {
      coeffsArg.append(coeffs[i]);
      inputsArg.append(std::get<IntVar>(_model->var(inputIdentifiers.at(i))));
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
      varBounds.emplace_back(std::pair<Int, Int>(-2, 2));
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

static Int clamp(Int val, Int numInputs) {
  return std::max(std::numeric_limits<Int>::min() / numInputs,
                  std::min(std::numeric_limits<Int>::max() / numInputs, val));
}

static std::pair<Int, Int> computeLinearBounds(
    const std::vector<Int>& coeffs,
    const std::vector<std::pair<Int, Int>>& bounds) {
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < coeffs.size(); ++i) {
    Int v1 = coeffs[i] * bounds[i].first;
    Int v2 = coeffs[i] * bounds[i].second;
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }
  return std::pair<Int, Int>(lb, ub);
}

RC_GTEST_FIXTURE_PROP(int_lin_leTest, rapidcheck,
                      (std::vector<Int> coefficients, Int bMax)) {
  const size_t numInputs = coefficients.size();
  bound = bMax;

  std::random_device rd;
  std::mt19937 gen(rd());
  coeffs.clear();
  coeffs.reserve(numInputs);
  std::copy(coefficients.begin(), coefficients.end(),
            std::back_inserter(coeffs));
  if (coeffs.size() == 2 && (coeffs.front() < 0 || coeffs.back() < 0)) {
    bound = bMax;
  }
  varBounds.reserve(numInputs);
  for (size_t i = 0; i < numInputs; ++i) {
    Int v1, v2;
    coeffs[i] = clamp(coeffs[i], numInputs);
    if (coeffs[i] == 0) {
      v1 = 0;
      v2 = 0;
    } else {
      v1 = std::numeric_limits<Int>::min() / (-std::abs(coeffs[i]) * numInputs);
      v2 = std::numeric_limits<Int>::max() / (std::abs(coeffs[i]) * numInputs);
    }
    if (v1 == v2) {
      varBounds.emplace_back(std::pair<Int, Int>(v1, v2));
    } else {
      std::uniform_int_distribution<Int> distr(v1, v2);
      const Int b1 = distr(gen);
      const Int b2 = distr(gen);
      varBounds.emplace_back(
          std::pair<Int, Int>(std::min(b1, b2), std::max(b1, b2)));
    }
  }

  generate();

  const auto [lb, ub] = computeLinearBounds(coeffs, varBounds);

  if (lb > bound) {
    RC_ASSERT_THROWS_AS(
        int_lin_le(*_invariantGraph, _model->constraints().back()),
        FznArgumentException);
    return;
  }
  RC_ASSERT(int_lin_le(*_invariantGraph, _model->constraints().back()));

  _invariantGraph->apply(*_solver);

  if (ub <= bound) {
    RC_ASSERT(totalViolationVarId() == propagation::NULL_ID);
    return;
  }

  for (size_t i = 0; i < numInputs; ++i) {
    RC_ASSERT(_invariantGraph->varNodeId(inputIdentifiers[i]) != NULL_NODE_ID);
  }

  RC_ASSERT(totalViolationVarId() != propagation::NULL_ID);
  Int sum = 0;
  _solver->beginMove();
  for (size_t i = 0; i < numInputs; ++i) {
    std::uniform_int_distribution<Int> distr(varBounds[i].first,
                                             varBounds[i].second);
    const Int val = distr(gen);
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