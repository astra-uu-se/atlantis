#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/intModViewNode.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntModViewNodeTestFixture : public NodeTestBase<IntModViewNode> {
 public:
  std::string outputVar{"output"};
  std::string inputVar{"input"};

  Int denominator{5};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      return _solver->currentValue(varId(inputVar)) % std::abs(denominator);
    }
    return varNode(inputVar).domain()->lowerBound() % std::abs(denominator);
  }

  void SetUp() {
    NodeTestBase::SetUp();
    const Int lb = shouldBeSubsumed() ? 5 : -10;
    const Int ub = shouldBeSubsumed() ? 5 : 10;
    retrieveIntVarNode(lb, ub, inputVar);
    retrieveIntVarNode(0, 5, outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(inputVar),
                        varNodeId(outputVar), denominator);
  }
};

TEST_P(IntModViewNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(inputVar).isFixed());
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).domain()->lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(IntModViewNodeTestFixture, propagation) {
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

    expectVarVals({inputId}, {inputVal});

    const Int expected = computeOutput(true);
    const Int actual = _solver->currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    IntModViewNodeTest, IntModViewNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
