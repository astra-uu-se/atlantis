#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntModNodeTestFixture : public NodeTestBase<IntModNode> {
 public:
  VarNodeId numeratorVarNodeId{NULL_NODE_ID};
  VarNodeId denominatorVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int numerator =
          varNode(numeratorVarNodeId).isFixed()
              ? varNode(numeratorVarNodeId).lowerBound()
              : _solver->currentValue(varId(numeratorVarNodeId));
      const Int denominator =
          varNode(denominatorVarNodeId).isFixed()
              ? varNode(denominatorVarNodeId).lowerBound()
              : _solver->currentValue(varId(denominatorVarNodeId));
      return denominator != 0 ? numerator % denominator : 0;
    }
    const Int numerator = varNode(numeratorVarNodeId).lowerBound();
    const Int denominator = varNode(denominatorVarNodeId).lowerBound();
    return denominator != 0 ? numerator % denominator : 0;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    numeratorVarNodeId = retrieveIntVarNode(0, 6, "numerator");
    denominatorVarNodeId = retrieveIntVarNode(1, 10, "denominator");
    outputVarNodeId = retrieveIntVarNode(0, 10, outputIdentifier);

    createInvariantNode(*_invariantGraph, numeratorVarNodeId,
                        denominatorVarNodeId, outputVarNodeId);
  }
};

TEST_P(IntModNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().numerator(), numeratorVarNodeId);
  EXPECT_EQ(invNode().denominator(), denominatorVarNodeId);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntModNodeTestFixture, application) {
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

  // numeratorVarNodeId and denominatorVarNodeId
  EXPECT_EQ(_solver->searchVars().size(), 2);

  // numeratorVarNodeId, denominatorVarNodeId and outputVarNodeId
  EXPECT_EQ(_solver->numVars(), 3);

  // intMod
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(IntModNodeTestFixture, propagation) {
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
       std::array<VarNodeId, 2>{numeratorVarNodeId, denominatorVarNodeId}) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarViewId outputId =
      varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputVarIds.size(), 2);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    if (inputVals.at(1) != 0) {
      const Int actual = _solver->currentValue(outputId);
      const Int expected = computeOutput(true);
      EXPECT_EQ(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(IntModNodeTest, IntModNodeTestFixture,
                        ::testing::Values(ParamData{
                            InvariantNodeAction::NONE}));

}  // namespace atlantis::testing
