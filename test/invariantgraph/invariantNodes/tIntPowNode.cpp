#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntPowNodeTestFixture : public NodeTestBase<IntPowNode> {
 public:
  VarNodeId baseVarNodeId{NULL_NODE_ID};
  VarNodeId exponentVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int int_exp(Int baseVal, Int exponentVal) {
    if (exponentVal == 0) {
      return 1;
    }
    if (exponentVal == 1) {
      return baseVal;
    }
    if (exponentVal < 0) {
      EXPECT_NE(baseVal, 0);
      if (baseVal == 1) {
        return 1;
      }
      if (baseVal == -1) {
        return exponentVal % 2 == 0 ? 1 : -1;
      }
      return 0;
    }
    Int result = 1;
    for (int i = 0; i < exponentVal; i++) {
      result *= baseVal;
    }
    return result;
  }

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int baseVal = varNode(baseVarNodeId).isFixed()
                              ? varNode(baseVarNodeId).lowerBound()
                              : _solver->currentValue(varId(baseVarNodeId));
      const Int exponentVal =
          varNode(exponentVarNodeId).isFixed()
              ? varNode(exponentVarNodeId).lowerBound()
              : _solver->currentValue(varId(exponentVarNodeId));
      return int_exp(baseVal, exponentVal);
    }
    const Int baseVal = varNode(baseVarNodeId).lowerBound();
    const Int exponentVal = varNode(exponentVarNodeId).lowerBound();
    return int_exp(baseVal, exponentVal);
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    baseVarNodeId = retrieveIntVarNode(0, 10, "base");
    exponentVarNodeId = retrieveIntVarNode(0, 10, "exponent");
    outputVarNodeId = retrieveIntVarNode(0, 10, outputIdentifier);

    createInvariantNode(*_invariantGraph, baseVarNodeId, exponentVarNodeId,
                        outputVarNodeId);
  }
};

TEST_P(IntPowNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().base(), baseVarNodeId);
  EXPECT_EQ(invNode().exponent(), exponentVarNodeId);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntPowNodeTestFixture, application) {
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

  // baseVarNodeId and exponentVarNodeId
  EXPECT_EQ(_solver->searchVars().size(), 2);

  // baseVarNodeId, exponentVarNodeId and outputVarNodeId
  EXPECT_EQ(_solver->numVars(), 3);

  // intPow
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(IntPowNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(true);
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& inputVarNodeId :
       std::array<VarNodeId, 2>{baseVarNodeId, exponentVarNodeId}) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
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

    if (inputVals.at(0) != 0 || inputVals.at(1) > 0) {
      const Int actual = _solver->currentValue(outputId);
      const Int expected = computeOutput(true);
      EXPECT_EQ(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(IntPowNodeTest, IntPowNodeTestFixture,
                        ::testing::Values(ParamData{
                            InvariantNodeAction::NONE}));

}  // namespace atlantis::testing
