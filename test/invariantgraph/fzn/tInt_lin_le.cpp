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

  Int isViolated() {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      EXPECT_TRUE(_invariantGraph->containsVarNode(inputIdentifiers.at(i)));
      const VarNode& vNode = _invariantGraph->varNode(inputIdentifiers.at(i));
      if (vNode.isFixed()) {
        sum += coeffs.at(i) * vNode.lowerBound();
      } else {
        EXPECT_NE(vNode.varId(), propagation::NULL_ID);
        Int curValue = _solver->currentValue(vNode.varId());
        if (!vNode.inDomain(curValue)) {
          return true;
        }
        sum += coeffs.at(i) * curValue;
      }
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
  _invariantGraph->construct();
  _invariantGraph->close();

  std::vector<propagation::VarViewId> inputVarIds = getVarIds(inputIdentifiers);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(totalViolationVarId());
    _solver->endProbe();

    const bool actual = violation() > 0;
    const bool expected = isViolated();

    EXPECT_EQ(actual, expected);
  }
}

}  // namespace atlantis::testing
