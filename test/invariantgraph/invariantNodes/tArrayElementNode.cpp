#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayElementNodeTestFixture : public NodeTestBase<ArrayElementNode> {
 public:
  std::string idxVar{"idx"};
  std::string outputVar{"output"};

  Int offsetIdx = 1;

  std::vector<Int> parArray{-2, -1, 0, 1};

  [[nodiscard]] static bool intParToBool(const Int val) {
    return std::abs(val) % 2 == 0;
  }

  [[nodiscard]] Int parVal(const Int val) const {
    return isIntElement() ? val : intParToBool(val) ? 0 : 1;
  }

  [[nodiscard]] bool isIntElement() const { return _paramData.data == 0; }

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      EXPECT_TRUE(varNode(idxVar).isFixed() ||
                  varId(idxVar) != propagation::NULL_ID);
      return parVal(parArray.at((varNode(idxVar).isFixed()
                                     ? varNode(idxVar).lowerBound()
                                     : _solver->currentValue(varId(idxVar))) -
                                offsetIdx));
    }
    EXPECT_TRUE(varNode(idxVar).isFixed());
    return parVal(parArray.at(varNode(idxVar).lowerBound() - offsetIdx));
  }

  void SetUp() {
    NodeTestBase::SetUp();
    retrieveIntVarNode(
        offsetIdx,
        shouldBeSubsumed()
            ? offsetIdx
            : (offsetIdx + static_cast<Int>(parArray.size()) - 1),
        idxVar);

    if (isIntElement()) {
      // int version of element
      retrieveIntVarNode(-2, 1, outputVar);
      createInvariantNode(*_invariantGraph, std::vector<Int>{parArray},
                          varNodeId(idxVar), varNodeId(outputVar), offsetIdx);
    } else {
      // bool version of element
      retrieveBoolVarNode(outputVar);
      std::vector<bool> boolArray(parArray.size());
      boolArray.reserve(parArray.size());
      for (size_t i = 0; i < parArray.size(); ++i) {
        boolArray.at(i) = intParToBool(parArray.at(i));
      }
      createInvariantNode(*_invariantGraph, std::move(boolArray),
                          varNodeId(idxVar), varNodeId(outputVar), offsetIdx);
    }
  }
};

TEST_P(ArrayElementNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), varNodeId(idxVar));
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), varNodeId(outputVar));

  std::vector<Int> expectedAs(parArray.size());
  for (size_t i = 0; i < parArray.size(); ++i) {
    expectedAs.at(i) = parVal(parArray.at(i));
  }
  EXPECT_EQ(invNode().as(), expectedAs);
}

TEST_P(ArrayElementNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(ArrayElementNodeTestFixture, propagation) {
  _invariantGraph->construct();
  _invariantGraph->close();

  VarNode& outputNode = varNode(outputVar);
  if (outputNode.isFixed()) {
    const Int actual = varNode(outputVar).lowerBound();
    const Int expected = computeOutput(true);

    EXPECT_EQ(expected, actual);
    return;
  }

  const propagation::VarViewId inputVarId = varId(idxVar);
  EXPECT_NE(inputVarId, propagation::NULL_ID);

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  for (Int inputVal = _solver->lowerBound(inputVarId);
       inputVal <= _solver->upperBound(inputVarId); ++inputVal) {
    _solver->beginMove();
    _solver->setValue(inputVarId, inputVal);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    const Int actual = _solver->currentValue(outputId);
    const Int expected = computeOutput(true);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayElementNodeTest, ArrayElementNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::REPLACE, 0}, ParamData{1},
                      ParamData{InvariantNodeAction::SUBSUME, 1},
                      ParamData{InvariantNodeAction::REPLACE, 1}));

}  // namespace atlantis::testing
