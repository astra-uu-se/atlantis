#include <algorithm>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_element.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayElementNodeTestFixture : public NodeTestBase<ArrayElementNode> {
 public:
  VarNodeId idx{NULL_NODE_ID};
  VarNodeId outputVar{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int offsetIdx = 1;

  std::vector<Int> parArray{-2, -1, 0, 1};

  bool intParToBool(const Int val) { return std::abs(val) % 2 == 0; }

  Int parVal(const Int val) {
    return isIntElement() ? val : intParToBool(val) ? 0 : 1;
  }

  bool isIntElement() const { return _mode <= 1; }
  bool shouldBeSubsumed() const { return _mode == 1 || _mode == 3; }

  void SetUp() override {
    NodeTestBase::SetUp();

    idx = retrieveIntVarNode(
        offsetIdx,
        shouldBeSubsumed()
            ? offsetIdx
            : (offsetIdx + static_cast<Int>(parArray.size()) - 1),
        "idx");

    if (isIntElement()) {
      // int version of element
      outputVar = retrieveIntVarNode(-2, 1, outputIdentifier);
      createInvariantNode(std::vector<Int>{parArray}, idx, outputVar,
                          offsetIdx);
    } else {
      // bool version of element
      outputVar = retrieveBoolVarNode(outputIdentifier);
      std::vector<bool> boolArray(parArray.size());
      boolArray.reserve(parArray.size());
      for (size_t i = 0; i < parArray.size(); ++i) {
        boolArray.at(i) = intParToBool(parArray.at(i));
      }
      createInvariantNode(std::move(boolArray), idx, outputVar, offsetIdx);
    }
  }
};

TEST_P(ArrayElementNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVar);

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
  EXPECT_GE(solver.lowerBound(varId(idx)), offsetIdx);
  EXPECT_LE(solver.upperBound(varId(idx)),
            offsetIdx + static_cast<Int>(invNode().as().size()) - 1);

  // The outputVar domain should contain all elements in as.
  if (isIntElement()) {
    EXPECT_GE(solver.lowerBound(varId(outputVar)),
              *std::min_element(parArray.begin(), parArray.end()));
    EXPECT_LE(solver.upperBound(varId(outputVar)),
              *std::max_element(parArray.begin(), parArray.end()));
  } else {
    EXPECT_GE(solver.lowerBound(varId(outputVar)), 0);
    EXPECT_LE(solver.upperBound(varId(outputVar)), 1);
  }

  // outputVar
  EXPECT_EQ(solver.searchVars().size(), 1);

  // outputVar (outputVar is a view)
  EXPECT_EQ(solver.numVars(), 1);

  // elementConst is a view
  EXPECT_EQ(solver.numInvariants(), 0);
}

TEST_P(ArrayElementNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int idxValue = varNode(idx).lowerBound();
    const Int expected = parVal(parArray.at(idxValue - offsetIdx));
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(ArrayElementNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputs;
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputs.emplace_back(varId(inputVarNodeId));
  }

  VarNode& outputNode = varNode(outputIdentifier);
  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual =
        parVal(parArray.at(varNode(idx).lowerBound() - offsetIdx));
    EXPECT_EQ(expected, actual);
    return;
  }

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);
  EXPECT_EQ(inputs.size(), 1);

  const propagation::VarId inputVar = inputs.front();

  for (Int inputVal = solver.lowerBound(inputVar);
       inputVal <= solver.upperBound(inputVar); ++inputVal) {
    solver.beginMove();
    solver.setValue(inputVar, inputVal);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);

    EXPECT_EQ(actual, parVal(parArray.at(inputVal - offsetIdx)));
  }
}

INSTANTIATE_TEST_CASE_P(ArrayElementNodeTest, ArrayElementNodeTestFixture,
                        ::testing::Values(0, 1, 2, 3));

}  // namespace atlantis::testing
