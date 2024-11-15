#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElementNodeTestFixture
    : public NodeTestBase<ArrayVarElementNode> {
 public:
  std::vector<Var> varArray;

  Var idxVar{NULL_NODE_ID, "idx"};
  Var outputVar{NULL_NODE_ID, "output"};

  Int offsetIdx = 1;

  [[nodiscard]] bool isIntElement() const { return _paramData.data == 0; }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (isIntElement()) {
      varArray = {makeIntVar(-2, 0, "x1"), makeIntVar(-1, 1, "x2"),
                  makeIntVar(0, 2, "x3")};
      outputVar.id = retrieveIntVarNode(-2, 2, outputVar.identifier);
    } else {
      varArray = {makeBoolVar("x1"), makeBoolVar("x2"), makeBoolVar("x3")};
      outputVar.id = retrieveBoolVarNode(outputVar.identifier);
    }

    idxVar.id = retrieveIntVarNode(
        offsetIdx,
        shouldBeReplaced()
            ? offsetIdx
            : (static_cast<Int>(varArray.size()) - 1 + offsetIdx),
        idxVar.identifier);

    createInvariantNode(*_invariantGraph, idxVar.id, varNodeIds(varArray),
                        outputVar.id, offsetIdx);
  }
};

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
    EXPECT_TRUE(varNode(idxVar).isFixed());
    EXPECT_FALSE(varNode(outputVar).isFixed());
    return;
  }

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarViewId> inputVarIds;
  std::vector<Int> inputVals;

  inputVarIds.emplace_back(varNode(idxVar).isFixed() ? propagation::NULL_ID
                                                     : varId(idxVar));
  inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                             ? varNode(idxVar).lowerBound()
                             : _solver->lowerBound(inputVarIds.back()));

  for (const auto& var : varArray) {
    inputVarIds.emplace_back(varNode(var).isFixed() ? propagation::NULL_ID
                                                    : varId(var));
    inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                               ? varNode(var).lowerBound()
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
