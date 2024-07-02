#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;

class BoolLinearNodeTestFixture : public NodeTestBase<BoolLinearNode> {
 public:
  size_t numInputs = 3;
  std::vector<VarNodeId> inputs{};
  std::vector<Int> coeffs{};
  VarNodeId outputVar{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(propagation::Solver& solver) {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (varNode(inputs.at(i)).isFixed()) {
        sum += varNode(inputs.at(i)).inDomain(bool{true}) ? coeffs.at(i) : 0;
      } else {
        sum += solver.currentValue(varId(inputs.at(i))) == 0 ? coeffs.at(i) : 0;
      }
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs.reserve(numInputs);
    coeffs.reserve(numInputs);
    Int minSum = 0;
    Int maxSum = 0;
    for (size_t i = 0; i < numInputs; ++i) {
      inputs.push_back(retrieveBoolVarNode("input" + std::to_string(i)));
      if (shouldBeSubsumed()) {
        _invariantGraph->varNode(inputs.back()).fixToValue(bool{i % 2 == 0});
      }
      coeffs.push_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min<Int>(coeffs.back(), 0);
      maxSum += std::max<Int>(coeffs.back(), 0);
    }

    outputVar = retrieveIntVarNode(minSum, maxSum, outputIdentifier);

    createInvariantNode(std::vector<Int>(coeffs),
                        std::vector<VarNodeId>(inputs), outputVar);
  }
};

TEST_P(BoolLinearNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(inputs));
  EXPECT_THAT(invNode().outputVarNodeIds(), std::vector<VarNodeId>{outputVar});
}

TEST_P(BoolLinearNodeTestFixture, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // inputs and outputVar
  EXPECT_EQ(solver.searchVars().size(), inputs.size());

  // inputs and outputVar
  EXPECT_EQ(solver.numVars(), inputs.size() + 1);

  // linear invariant
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(BoolLinearNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar).isFixed());
    Int expected = 0;
    for (size_t i = 0; i < inputs.size(); ++i) {
      expected += varNode(inputs.at(i)).lowerBound() == 0 ? coeffs.at(i) : 0;
    }
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(BoolLinearNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  VarNode& outputNode = varNode(outputIdentifier);
  if (outputNode.isFixed()) {
    Int expected = 0;
    for (size_t i = 0; i < inputs.size(); ++i) {
      expected += varNode(inputs.at(i)).lowerBound() == 0 ? coeffs.at(i) : 0;
    }
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarId> inputVars;
  for (const auto& nId : inputs) {
    if (!varNode(nId).isFixed()) {
      EXPECT_NE(varId(nId), propagation::NULL_ID);
      inputVars.emplace_back(varId(nId));
    }
  }

  EXPECT_FALSE(inputVars.empty());

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(solver);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(BoolLinearNodeTest, BoolLinearNodeTestFixture,
                        ::testing::Values(ParamData{InvariantNodeAction::NONE},
                                          ParamData{
                                              InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
