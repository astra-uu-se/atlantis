#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntScalarNodeTestFixture : public NodeTestBase<IntScalarNode> {
 protected:
  Var outputVar{"output", std::vector<Int>{}, true};
  Var inputVar{"input", std::vector<Int>{}, true};

  Int factor{2};
  Int offset{5};

  [[nodiscard]] Int computeOutput(const bool isRegistered = false) const {
    if (isRegistered) {
      return _solver->currentValue(varId(inputVar)) * factor + offset;
    }
    return varNodeConst(inputVar).lowerBound() * factor + offset;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    const Int lb = -10;
    const Int ub = 10;
    inputVar.domain = std::pair<Int, Int>{lb, ub};
    outputVar.domain =
        std::pair<Int, Int>{lb * factor + offset, ub * factor + offset};
    retrieveIntVarNode(inputVar);
    retrieveIntVarNode(outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(inputVar),
                        varNodeId(outputVar), factor, offset);
  }
};

TEST_P(IntScalarNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(inputVar).isFixed());
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).constDomain()->lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(IntScalarNodeTestFixture, propagation) {
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

    const Int expected = computeOutput(true);
    const Int actual = _solver->currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(IntScalarNodeTest, IntScalarNodeTestFixture,
                         ::testing::Values(ParamData{
                             InvariantNodeAction::NONE}));

}  // namespace atlantis::testing
