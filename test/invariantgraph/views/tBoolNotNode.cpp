#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolNotNodeTestFixture : public NodeTestBase<BoolNotNode> {
 protected:
  Var outputVar{"output", std::vector<Int>{}, false};
  Var inputVar{"input", std::vector<Int>{}, false};

  [[nodiscard]] bool computeOutput(const bool isRegistered = false) const {
    if (isRegistered) {
      return _solver->currentValue(varId(inputVar)) > 0;
    }
    return varNodeConst(inputVar).inDomain(bool{false});
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        inputVar.domain = std::vector<Int>{1};
      } else {
        outputVar.domain = std::vector<Int>{1};
      }
    }

    retrieveBoolVarNode(inputVar);
    retrieveBoolVarNode(outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(inputVar),
                        varNodeId(outputVar));
  }
};

TEST_P(BoolNotNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(inputVar).isFixed());
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const bool expected = computeOutput();
    const bool actual = varNode(outputVar).inDomain(bool{true});
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(BoolNotNodeTestFixture, propagation) {
  if (shouldBeSubsumed()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

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

    const bool expected = computeOutput(true);
    const bool actual = _solver->currentValue(outputId) == 0;
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    BoolNotNodeTest, BoolNotNodeTestFixture,
    ::testing::Values(ParamData{},
                      ParamData{InvariantNodeAction::REPLACE, int{0}},
                      ParamData{InvariantNodeAction::REPLACE, int{1}}));

}  // namespace atlantis::testing
