#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
  Int numInputs = 4;
  std::vector<std::string> inputIdentifiers{};
  std::vector<Int> coeffs{};
  Int bound = 0;

  Int isViolated(const std::vector<Int>& inputVals) {
    Int sum = 0;
    for (Int i = 0; i < numInputs; ++i) {
      sum += coeffs.at(i) * inputVals.at(i);
    }
    return sum > bound;
  }

  void SetUp() override {
    FznTestBase::SetUp();
    constraintIdentifier = "int_lin_le";

    std::vector<Arg> args;
    args.reserve(2);

    inputIdentifiers.reserve(numInputs);
    coeffs.reserve(numInputs);
    IntVarArray coeffsArg("coeffs");
    IntVarArray inputsArg("inputs");
    for (Int i = 0; i < numInputs; ++i) {
      coeffs.emplace_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      coeffsArg.append(coeffs.back());

      inputIdentifiers.emplace_back("input" + std::to_string(i));
      _model->addVar(std::move(IntVar(-2, 2, inputIdentifiers.back())));
      inputsArg.append(std::get<IntVar>(_model->var(inputIdentifiers.back())));
    }
    args.emplace_back(coeffsArg);
    args.emplace_back(inputsArg);
    args.emplace_back(IntArg(bound));

    _model->addConstraint(Constraint(constraintIdentifier, std::move(args)));
  }
};

TEST_F(int_lin_leTest, construction) {
  EXPECT_EQ(_model->constraints().size(), 1);
  EXPECT_TRUE(int_lin_le(*_invariantGraph, _model->constraints().front()));
  for (const auto& identifier : inputIdentifiers) {
    EXPECT_TRUE(_invariantGraph->containsVarNode(identifier));
    EXPECT_NE(_invariantGraph->varNodeId(identifier), NULL_NODE_ID);
  }
}

TEST_F(int_lin_leTest, propagation) {
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

}  // namespace atlantis::testing