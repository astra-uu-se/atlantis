#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolNotNodeTestFixture : public NodeTestBase<BoolNotNode> {
 public:
  std::string outputVar{"output"};
  std::string inputVar{"input"};

  bool computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      return _solver->currentValue(varId(inputVar)) > 0;
    }
    return varNode(inputVar).inDomain(bool{false});
  }

  void generate() {
    retrieveBoolVarNode(inputVar);
    retrieveBoolVarNode(outputVar);

    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        varNode(inputVar).fixToValue(bool{true});
      } else {
        varNode(outputVar).fixToValue(bool{true});
      }
    }

    createInvariantNode(*_invariantGraph, varNodeId(inputVar),
                        varNodeId(outputVar));
  }
};

TEST_P(BoolNotNodeTestFixture, updateState) {
  generate();
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
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
  generate();
  if (shouldBeSubsumed()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

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
