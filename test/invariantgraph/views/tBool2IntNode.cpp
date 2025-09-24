#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/bool2IntNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class Bool2IntNodeTestFixture : public NodeTestBase<Bool2IntNode> {
 public:
  std::string outputVar{"output"};
  std::string inputVar{"input"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      return _solver->currentValue(varId(inputVar)) == 0 ? 1 : 0;
    }
    return varNode(inputVar).inDomain(bool{true}) ? 1 : 0;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    retrieveBoolVarNode(inputVar);
    retrieveIntVarNode(0, 1, outputVar);

    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        varNode(inputVar).fixToValue(bool{true});
      } else {
        varNode(outputVar).fixToValue(Int{0});
      }
    }

    createInvariantNode(*_invariantGraph, varNodeId(inputVar),
                        varNodeId(outputVar));
  }
};

TEST_P(Bool2IntNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(inputVar).isFixed());
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(Bool2IntNodeTestFixture, propagation) {
  if (shouldBeSubsumed()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping = std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  const propagation::VarViewId inputId = varId(inputVar);
  EXPECT_NE(inputId, propagation::NULL_ID);

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  for (Int inputVal = _solver->lowerBound(inputId);
       inputVal <= _solver->upperBound(inputId); ++inputVal) {
    _solver->beginMove();
    _solver->setValue(inputId, inputVal);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    const Int expected = computeOutput(true);
    const Int actual = _solver->currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Bool2IntNodeTest, Bool2IntNodeTestFixture,
    ::testing::Values(ParamData{},
                      ParamData{InvariantNodeAction::REPLACE, int{0}},
                      ParamData{InvariantNodeAction::REPLACE, int{1}}));

}  // namespace atlantis::testing
