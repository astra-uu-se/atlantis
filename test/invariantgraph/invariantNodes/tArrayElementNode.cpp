#include <algorithm>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_element.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayElementNodeTestFixture : public NodeTestBase<ArrayElementNode> {
 public:
  VarNodeId idxVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int offsetIdx = 1;

  std::vector<Int> parArray{-2, -1, 0, 1};

  bool intParToBool(const Int val) { return std::abs(val) % 2 == 0; }

  Int parVal(const Int val) {
    return isIntElement() ? val : intParToBool(val) ? 0 : 1;
  }

  bool isIntElement() const { return _paramData.data == 0; }

  Int computeOutput() {
    EXPECT_TRUE(varNode(idxVarNodeId).isFixed());
    return parVal(parArray.at(varNode(idxVarNodeId).lowerBound() - offsetIdx));
  }

  Int computeOutput(propagation::Solver& solver) {
    EXPECT_TRUE(varNode(idxVarNodeId).isFixed() ||
                varId(idxVarNodeId) != propagation::NULL_ID);
    return parVal(parArray.at((varNode(idxVarNodeId).isFixed()
                                   ? varNode(idxVarNodeId).lowerBound()
                                   : solver.currentValue(varId(idxVarNodeId))) -
                              offsetIdx));
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    idxVarNodeId = retrieveIntVarNode(
        offsetIdx,
        shouldBeSubsumed()
            ? offsetIdx
            : (offsetIdx + static_cast<Int>(parArray.size()) - 1),
        "idx");

    if (isIntElement()) {
      // int version of element
      outputVarNodeId = retrieveIntVarNode(-2, 1, outputIdentifier);
      createInvariantNode(std::vector<Int>{parArray}, idxVarNodeId,
                          outputVarNodeId, offsetIdx);
    } else {
      // bool version of element
      outputVarNodeId = retrieveBoolVarNode(outputIdentifier);
      std::vector<bool> boolArray(parArray.size());
      boolArray.reserve(parArray.size());
      for (size_t i = 0; i < parArray.size(); ++i) {
        boolArray.at(i) = intParToBool(parArray.at(i));
      }
      createInvariantNode(std::move(boolArray), idxVarNodeId, outputVarNodeId,
                          offsetIdx);
    }
  }
};

TEST_P(ArrayElementNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), idxVarNodeId);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);

  std::vector<Int> expectedAs(parArray.size());
  for (size_t i = 0; i < parArray.size(); ++i) {
    expectedAs.at(i) = parVal(parArray.at(i));
  }
  EXPECT_EQ(invNode().as(), expectedAs);
}

TEST_P(ArrayElementNodeTestFixture, application) {
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

  // The index ranges over the as array (first index is 1).
  EXPECT_GE(solver.lowerBound(varId(idxVarNodeId)), offsetIdx);
  EXPECT_LE(solver.upperBound(varId(idxVarNodeId)),
            offsetIdx + static_cast<Int>(invNode().as().size()) - 1);

  // The outputVarNodeId domain should contain all elements in as.
  if (isIntElement()) {
    EXPECT_GE(solver.lowerBound(varId(outputVarNodeId)),
              *std::min_element(parArray.begin(), parArray.end()));
    EXPECT_LE(solver.upperBound(varId(outputVarNodeId)),
              *std::max_element(parArray.begin(), parArray.end()));
  } else {
    EXPECT_GE(solver.lowerBound(varId(outputVarNodeId)), 0);
    EXPECT_LE(solver.upperBound(varId(outputVarNodeId)), 1);
  }

  // outputVarNodeId
  EXPECT_EQ(solver.searchVars().size(), 1);

  // outputVarNodeId (outputVarNodeId is a view)
  EXPECT_EQ(solver.numVars(), 1);

  // elementConst is a view
  EXPECT_EQ(solver.numInvariants(), 0);
}

TEST_P(ArrayElementNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(ArrayElementNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  VarNode& outputNode = varNode(outputIdentifier);
  if (outputNode.isFixed()) {
    const Int actual = varNode(outputVarNodeId).lowerBound();
    const Int expected = computeOutput(solver);

    EXPECT_EQ(expected, actual);
    return;
  }

  const propagation::VarId inputVarId = varId(idxVarNodeId);
  EXPECT_NE(inputVarId, propagation::NULL_ID);

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  for (Int inputVal = solver.lowerBound(inputVarId);
       inputVal <= solver.upperBound(inputVarId); ++inputVal) {
    solver.beginMove();
    solver.setValue(inputVarId, inputVal);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(solver);

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
