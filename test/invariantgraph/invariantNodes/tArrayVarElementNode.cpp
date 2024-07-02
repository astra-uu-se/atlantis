#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElementNodeTestFixture
    : public NodeTestBase<ArrayVarElementNode> {
 public:
  std::vector<VarNodeId> varArray;

  VarNodeId idx{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int offsetIdx = 1;

  bool isIntElement() const { return _paramData.data == 0; }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (isIntElement()) {
      varArray = {retrieveIntVarNode(-2, 0, "x1"),
                  retrieveIntVarNode(-1, 1, "x2"),
                  retrieveIntVarNode(0, 2, "x3")};
      output = retrieveIntVarNode(-2, 2, outputIdentifier);
    } else {
      varArray = {retrieveBoolVarNode("x1"), retrieveBoolVarNode("x2"),
                  retrieveBoolVarNode("x3")};
      output = retrieveBoolVarNode(outputIdentifier);
    }

    idx = retrieveIntVarNode(
        offsetIdx,
        shouldBeReplaced()
            ? offsetIdx
            : (static_cast<Int>(varArray.size()) - 1 + offsetIdx),
        "idx");

    createInvariantNode(idx, std::vector<VarNodeId>{varArray}, output,
                        offsetIdx);
  }
};

TEST_P(ArrayVarElementNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), varArray.size());
  for (size_t i = 0; i < varArray.size(); ++i) {
    EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(i), varArray.at(i));
  }
}

TEST_P(ArrayVarElementNodeTestFixture, application) {
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

  // x1, x2, x3, idx
  EXPECT_EQ(solver.searchVars().size(), 4);

  // x1, x2, x3, idx, output
  EXPECT_EQ(solver.numVars(), 5);

  // elementVar
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(ArrayVarElementNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_TRUE(invNode().replace(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  }
}

TEST_P(ArrayVarElementNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarId> inputVars;
  std::vector<Int> inputVals;

  inputVars.emplace_back(varNode(idx).isFixed() ? propagation::NULL_ID
                                                : varId(idx));
  inputVals.emplace_back(inputVars.back() == propagation::NULL_ID
                             ? varNode(idx).lowerBound()
                             : solver.lowerBound(inputVars.back()));

  for (const auto& nId : varArray) {
    inputVars.emplace_back(varNode(nId).isFixed() ? propagation::NULL_ID
                                                  : varId(nId));
    inputVals.emplace_back(inputVars.back() == propagation::NULL_ID
                               ? varNode(nId).lowerBound()
                               : solver.lowerBound(inputVars.back()));
  }

  EXPECT_EQ(inputVars.size(), inputVals.size());

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);

    const Int index = inputVals.at(0) - offsetIdx + 1;
    const Int expected = inputVals.at(index);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayVarElementNodeTest, ArrayVarElementNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE, 0},
                      ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::REPLACE, 0},
                      ParamData{InvariantNodeAction::NONE, 1},
                      ParamData{InvariantNodeAction::SUBSUME, 1},
                      ParamData{InvariantNodeAction::REPLACE, 1}));

}  // namespace atlantis::testing
