#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMaximumNodeTestFixture
    : public NodeTestBase<ArrayIntMaximumNode> {
 public:
  Int numInputs = 3;
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers;
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int val = std::numeric_limits<Int>::min();
      for (const auto& identifier : inputIdentifiers) {
        if (varNode(identifier).isFixed() ||
            varId(identifier) == propagation::NULL_ID) {
          val = std::max(val, varNode(identifier).upperBound());
        } else {
          val = std::max(val, _solver->currentValue(varId(identifier)));
        }
      }
      return val;
    }
    Int val = std::numeric_limits<Int>::min();
    for (const auto& identifier : inputIdentifiers) {
      val = std::max(val, varNode(identifier).upperBound());
    }
    return val;
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    std::vector<std::pair<Int, Int>> bounds;

    if (shouldBeSubsumed()) {
      bounds = {{-5, 2}, {-2, 5}, {5, 5}};
    } else if (shouldBeReplaced()) {
      bounds = {{-2, 5}, {-5, -2}, {-2, -2}};
    } else {
      bounds = {{0, 5}, {2, 2}, {-5, 0}};
    }
    for (const auto& [lb, ub] : bounds) {
      inputIdentifiers.emplace_back("input_" +
                                    std::to_string(inputIdentifiers.size()));
      inputVarNodeIds.emplace_back(
          retrieveIntVarNode(lb, ub, inputIdentifiers.back()));
    }
    outputVarNodeId = retrieveIntVarNode(-5, 5, outputIdentifier);

    createInvariantNode(*_invariantGraph,
                        std::vector<VarNodeId>{inputVarNodeIds},
                        outputVarNodeId);
  }
};

TEST_P(ArrayIntMaximumNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputVarNodeIds.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(ArrayIntMaximumNodeTestFixture, application) {
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

  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();

  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    lb = std::max(lb, _solver->lowerBound(varId(inputVarNodeId)));
    ub = std::max(ub, _solver->upperBound(varId(inputVarNodeId)));
  }

  EXPECT_EQ(_solver->lowerBound(varId(outputVarNodeId)), lb);
  EXPECT_EQ(_solver->upperBound(varId(outputVarNodeId)), ub);

  // x1, x2, and x3
  EXPECT_EQ(_solver->searchVars().size(), 3);

  // x1, x2 and outputVarNodeId
  EXPECT_EQ(_solver->numVars(), 4);

  // max
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(ArrayIntMaximumNodeTestFixture, updateState) {
  Int minVal = std::numeric_limits<Int>::max();
  Int maxVal = std::numeric_limits<Int>::min();
  for (const auto& identifier : inputIdentifiers) {
    minVal =
        std::min(minVal, _invariantGraph->varNode(identifier).lowerBound());
    maxVal =
        std::max(maxVal, _invariantGraph->varNode(identifier).upperBound());
  }
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    // EXPECT_TRUE(_invariantGraph->varNode(outputVarNodeId).isFixed());
    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual =
        _invariantGraph->varNode(outputVarNodeId).lowerBound();
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
  } else {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_LE(minVal, _invariantGraph->varNode(outputVarNodeId).lowerBound());
  EXPECT_GE(maxVal, _invariantGraph->varNode(outputVarNodeId).upperBound());
}

TEST_P(ArrayIntMaximumNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeReplaced()) {
    EXPECT_TRUE(invNode().canBeReplaced());
    EXPECT_TRUE(invNode().replace());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else if (invNode().state() == InvariantNodeState::ACTIVE) {
    EXPECT_FALSE(invNode().canBeReplaced());
    EXPECT_FALSE(invNode().replace());
  }
}

TEST_P(ArrayIntMaximumNodeTestFixture, propagation) {
  Int lb = std::numeric_limits<Int>::min();
  for (const auto& identifier : inputIdentifiers) {
    lb = std::max(lb, varNode(identifier).lowerBound());
  }

  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    [[maybe_unused]] const Int expected = computeOutput(true);
    [[maybe_unused]] const Int actual = varNode(outputVarNodeId).lowerBound();
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
    return;
  }
  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputIdentifier).isFixed());
    EXPECT_EQ(varId(outputIdentifier), propagation::NULL_ID);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& identifier : inputIdentifiers) {
    if (varNode(identifier).upperBound() > lb) {
      EXPECT_NE(varId(identifier), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifier));
    }
  }

  VarNode& outputNode = varNode(outputIdentifier);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual = computeOutput(true);
    EXPECT_EQ(expected, actual);
    return;
  }

  EXPECT_NE(varId(outputIdentifier), propagation::NULL_ID);

  const propagation::VarViewId outputId = varId(outputIdentifier);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int expected = computeOutput(true);
    const Int actual = _solver->currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayIntMaximumNodeTest, ArrayIntMaximumNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
