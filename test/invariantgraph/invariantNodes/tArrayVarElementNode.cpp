#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElementNodeTestFixture
    : public NodeTestBase<ArrayVarElementNode> {
 public:
  std::vector<VarNodeId> varArrayVarNodeIds;

  VarNodeId idx{NULL_NODE_ID};
  std::string idxIdentifier{"idx"};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int offsetIdx = 1;

  bool isIntElement() const { return _paramData.data == 0; }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (isIntElement()) {
      varArrayVarNodeIds = {retrieveIntVarNode(-2, 0, "x1"),
                            retrieveIntVarNode(-1, 1, "x2"),
                            retrieveIntVarNode(0, 2, "x3")};
      outputVarNodeId = retrieveIntVarNode(-2, 2, outputIdentifier);
    } else {
      varArrayVarNodeIds = {retrieveBoolVarNode("x1"),
                            retrieveBoolVarNode("x2"),
                            retrieveBoolVarNode("x3")};
      outputVarNodeId = retrieveBoolVarNode(outputIdentifier);
    }

    idx = retrieveIntVarNode(
        offsetIdx,
        shouldBeReplaced()
            ? offsetIdx
            : (static_cast<Int>(varArrayVarNodeIds.size()) - 1 + offsetIdx),
        idxIdentifier);

    createInvariantNode(*_invariantGraph, idx,
                        std::vector<VarNodeId>{varArrayVarNodeIds},
                        outputVarNodeId, offsetIdx);
  }
};

TEST_P(ArrayVarElementNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(),
            varArrayVarNodeIds.size());
  for (size_t i = 0; i < varArrayVarNodeIds.size(); ++i) {
    EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(i),
              varArrayVarNodeIds.at(i));
  }
}

TEST_P(ArrayVarElementNodeTestFixture, application) {
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

  // x1, x2, x3, idx
  EXPECT_EQ(_solver->searchVars().size(), 4);

  // x1, x2, x3, idx, outputVarNodeId
  EXPECT_EQ(_solver->numVars(), 5);

  // elementVar
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(ArrayVarElementNodeTestFixture, replace) {
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

TEST_P(ArrayVarElementNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeReplaced()) {
    EXPECT_TRUE(varNode(idxIdentifier).isFixed());
    EXPECT_FALSE(varNode(outputIdentifier).isFixed());
    return;
  }

  const propagation::VarViewId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarViewId> inputVarIds;
  std::vector<Int> inputVals;

  inputVarIds.emplace_back(varNode(idx).isFixed() ? propagation::NULL_ID
                                                  : varId(idx));
  inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                             ? varNode(idx).lowerBound()
                             : _solver->lowerBound(inputVarIds.back()));

  for (const auto& inputVarNodeId : varArrayVarNodeIds) {
    inputVarIds.emplace_back(varNode(inputVarNodeId).isFixed()
                                 ? propagation::NULL_ID
                                 : varId(inputVarNodeId));
    inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                               ? varNode(inputVarNodeId).lowerBound()
                               : _solver->lowerBound(inputVarIds.back()));
  }

  EXPECT_EQ(inputVarIds.size(), inputVals.size());

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int actual = _solver->currentValue(outputId);

    const Int index = inputVals.at(0) - offsetIdx + 1;
    const Int expected = inputVals.at(index);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayVarElementNodeTest, ArrayVarElementNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{InvariantNodeAction::REPLACE, 0},
                      ParamData{1},
                      ParamData{InvariantNodeAction::REPLACE, 1}));

}  // namespace atlantis::testing
