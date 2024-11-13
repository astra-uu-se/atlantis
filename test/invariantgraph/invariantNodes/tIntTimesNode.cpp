#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntTimesNodeTestFixture : public NodeTestBase<IntTimesNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers{"input_1", "input_2"};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int product = 1;
      for (const auto& identifier : inputIdentifiers) {
        if (varNode(identifier).isFixed()) {
          product *= varNode(identifier).lowerBound();
        } else {
          product *= _solver->currentValue(varId(identifier));
        }
      }
      return product;
    }
    Int product = 1;
    for (const auto& identifier : inputIdentifiers) {
      product *= varNode(identifier).lowerBound();
    }
    return product;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        inputVarNodeIds = {retrieveIntVarNode(0, 0, inputIdentifiers.front()),
                           retrieveIntVarNode(0, 10, inputIdentifiers.back())};
      } else {
        inputVarNodeIds = {retrieveIntVarNode(-2, -2, inputIdentifiers.front()),
                           retrieveIntVarNode(2, 2, inputIdentifiers.back())};
      }
    } else if (shouldBeReplaced()) {
      inputVarNodeIds = {retrieveIntVarNode(1, 1, inputIdentifiers.front()),
                         retrieveIntVarNode(-2, 2, inputIdentifiers.back())};
    } else {
      inputVarNodeIds = {retrieveIntVarNode(-2, 2, inputIdentifiers.front()),
                         retrieveIntVarNode(-2, 2, inputIdentifiers.back())};
    }
    outputVarNodeId = retrieveIntVarNode(-10, 10, outputIdentifier);

    createInvariantNode(*_invariantGraph, inputVarNodeIds.front(),
                        inputVarNodeIds.back(), outputVarNodeId);
  }
};

TEST_P(IntTimesNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputVarNodeIds.at(i));
  }
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntTimesNodeTestFixture, application) {
  _solver->open();
  addInputVarsToSolver();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  // a and b
  EXPECT_EQ(_solver->searchVars().size(), 2);

  // a, b and outputVarNodeId
  EXPECT_EQ(_solver->numVars(), 3);

  // intTimes
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(IntTimesNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    /*
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    TODO: disabled for the MZN challange. This should be computed by Gecode.
    EXPECT_EQ(expected, actual);
    */
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(IntTimesNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced());
    EXPECT_TRUE(invNode().replace());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced());
  }
}

TEST_P(IntTimesNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    /*
    VarNode& outputNode = varNode(outputVarNodeId);
    EXPECT_TRUE(outputNode.isFixed());
    const Int actual = outputNode.lowerBound();
    const Int expected = computeOutput();
    EXPECT_EQ(actual, expected);
    */
    return;
  }

  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputIdentifier).isFixed());
    const VarNodeId factorVarNodeId =
        varNode(varNode(inputIdentifiers.front()).isFixed()
                    ? inputIdentifiers.back()
                    : inputIdentifiers.front())
            .varNodeId();
    EXPECT_EQ(varNode(outputIdentifier).varNodeId(), factorVarNodeId);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& identifier : inputIdentifiers) {
    if (!varNode(identifier).isFixed()) {
      EXPECT_NE(varId(identifier), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifier));
    }
  }

  const propagation::VarViewId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int actual = _solver->currentValue(outputId);
    const Int expected = computeOutput(true);
    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    IntTimesNodeTest, IntTimesNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::SUBSUME, 1},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
