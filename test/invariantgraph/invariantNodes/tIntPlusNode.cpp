#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::Contains;

class IntPlusNodeTestFixture : public NodeTestBase<IntPlusNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers{"input_1", "input_2"};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int sum = 0;
      for (const auto& identifier : inputIdentifiers) {
        if (varNode(identifier).isFixed()) {
          sum += varNode(identifier).lowerBound();
        } else {
          sum += _solver->currentValue(varId(identifier));
        }
      }
      return sum;
    }
    Int sum = 0;
    for (const auto& identifier : inputIdentifiers) {
      EXPECT_TRUE(varNode(identifier).isFixed());
      sum += varNode(identifier).lowerBound();
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      inputVarNodeIds = std::vector<VarNodeId>{
          retrieveIntVarNode(1, 1, inputIdentifiers.front()),
          retrieveIntVarNode(1, 1, inputIdentifiers.back())};
    } else if (shouldBeReplaced()) {
      inputVarNodeIds = std::vector<VarNodeId>{
          retrieveIntVarNode(0, 0, inputIdentifiers.front()),
          retrieveIntVarNode(-2, 2, inputIdentifiers.back())};
    } else {
      inputVarNodeIds = std::vector<VarNodeId>{
          retrieveIntVarNode(-2, 2, inputIdentifiers.front()),
          retrieveIntVarNode(-2, 2, inputIdentifiers.back())};
    }
    outputVarNodeId = retrieveIntVarNode(0, 10, outputIdentifier);

    createInvariantNode(*_invariantGraph, inputVarNodeIds.front(),
                        inputVarNodeIds.back(), outputVarNodeId);
  }
};

TEST_P(IntPlusNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputVarNodeIds.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntPlusNodeTestFixture, application) {
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
  for (const auto& identifier : inputIdentifiers) {
    const VarNode& inputNode = varNode(identifier);
    if (shouldBeSubsumed()) {
      EXPECT_TRUE(inputNode.isFixed());
    } else if (!shouldBeReplaced()) {
      EXPECT_FALSE(inputNode.isFixed());
    }
    if (!inputNode.isFixed()) {
      EXPECT_TRUE(varId(identifier).isVar());
      EXPECT_THAT(_solver->searchVars(), Contains(size_t(varId(identifier))));
    }
  }
  EXPECT_LE(_solver->searchVars().size(), 2);

  if (!shouldBeSubsumed()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }

  // a, b and outputVarNodeId
  EXPECT_LE(_solver->numVars(), 3);

  // intPow
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(IntPlusNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(IntPlusNodeTestFixture, replace) {
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

TEST_P(IntPlusNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    VarNode& outputNode = varNode(outputIdentifier);
    EXPECT_TRUE(outputNode.isFixed());
    const Int actual = outputNode.lowerBound();
    const Int expected = computeOutput(true);
    EXPECT_EQ(actual, expected);
    return;
  }

  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputIdentifier).isFixed());
    const VarNodeId addantVarNodeId =
        varNode(varNode(inputIdentifiers.front()).isFixed()
                    ? inputIdentifiers.back()
                    : inputIdentifiers.front())
            .varNodeId();
    EXPECT_EQ(varNode(outputIdentifier).varNodeId(), addantVarNodeId);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& identifiers : inputIdentifiers) {
    if (!varNode(identifiers).isFixed()) {
      EXPECT_NE(varId(identifiers), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifiers));
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
    IntPlusNodeTest, IntPlusNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
