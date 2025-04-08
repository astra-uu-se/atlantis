#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElementNodeTestFixture
    : public NodeTestBase<ArrayVarElementNode> {
 public:
  std::vector<std::string> varArray;

  std::string idxVar{"idx"};
  std::string outputVar{"output"};

  Int offsetIdx = 1;

  [[nodiscard]] bool isIntElement() const { return _paramData.data == 0; }

  void SetUp() {
    NodeTestBase::SetUp();
    varArray = {"x1", "x2", "x3"};
    if (isIntElement()) {
      retrieveIntVarNode(-2, 0, varArray.at(0));
      retrieveIntVarNode(-1, 1, varArray.at(1));
      retrieveIntVarNode(0, 2, varArray.at(2));
      retrieveIntVarNode(-2, 2, outputVar);
    } else {
      for (const auto& identifier : varArray) {
        retrieveBoolVarNode(identifier);
      }
      retrieveBoolVarNode(outputVar);
    }

    retrieveIntVarNode(offsetIdx,
                       shouldBeReplaced() ? offsetIdx
                                          : (static_cast<Int>(varArray.size()) -
                                             1 + offsetIdx),
                       idxVar);

    createInvariantNode(*_invariantGraph, varNodeId(idxVar),
                        varNodeIds(varArray), varNodeId(outputVar), offsetIdx);
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
