#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_int_element2d.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayElement2dNodeTestFixture : public NodeTestBase<ArrayElement2dNode> {
 public:
  std::vector<std::vector<Int>> parMatrix{std::vector<Int>{-2, -1},
                                          std::vector<Int>{0, 1}};

  VarNodeId idx1{NULL_NODE_ID};
  VarNodeId idx2{NULL_NODE_ID};
  VarNodeId outputVar{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int offsetIdx1 = 1;
  Int offsetIdx2 = 1;

  bool intParToBool(const Int val) { return std::abs(val) % 2 == 0; }

  Int parVal(const Int val) {
    return isIntElement() ? val : intParToBool(val) ? 0 : 1;
  }

  bool isIntElement() const { return _mode <= 3; }
  bool shouldBeSubsumed() const { return _mode == 1 || _mode == 5; }
  bool idx1ShouldBeReplaced() const { return _mode == 2 || _mode == 6; }
  bool idx2ShouldBeReplaced() const { return _mode == 3 || _mode == 7; }
  bool shouldBeReplaced() const {
    return idx1ShouldBeReplaced() || idx2ShouldBeReplaced();
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    idx1 = retrieveIntVarNode(
        offsetIdx1,
        shouldBeSubsumed() || idx1ShouldBeReplaced()
            ? offsetIdx1
            : (offsetIdx1 + static_cast<Int>(parMatrix.size()) - 1),
        "idx1");
    idx2 = retrieveIntVarNode(
        offsetIdx2,
        shouldBeSubsumed() || idx2ShouldBeReplaced()
            ? offsetIdx2
            : (offsetIdx1 + static_cast<Int>(parMatrix.front().size()) - 1),
        "idx2");

    if (isIntElement()) {
      // int version of element
      outputVar = retrieveIntVarNode(-2, 1, outputIdentifier);
      createInvariantNode(idx1, idx2, std::vector<std::vector<Int>>{parMatrix},
                          outputVar, offsetIdx1, offsetIdx2);
    } else {
      // bool version of element
      outputVar = retrieveBoolVarNode(outputIdentifier);
      std::vector<std::vector<bool>> boolMatrix;
      boolMatrix.reserve(parMatrix.size());
      for (const auto& row : parMatrix) {
        boolMatrix.emplace_back();
        for (const auto val : row) {
          boolMatrix.back().emplace_back(intParToBool(val));
        }
      }
      createInvariantNode(idx1, idx2, std::move(boolMatrix), outputVar,
                          offsetIdx1, offsetIdx2);
    }
  }
};

TEST_P(ArrayElement2dNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1);
  EXPECT_EQ(invNode().idx2(), idx2);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVar);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
}

TEST_P(ArrayElement2dNodeTestFixture, application) {
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

  if (shouldBeSubsumed()) {
    EXPECT_EQ(solver.searchVars().size(), 1);
    EXPECT_EQ(solver.numVars(), 2);
  } else {
    EXPECT_EQ(solver.searchVars().size(), 2);
    EXPECT_EQ(solver.numVars(), 3);
  }
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(ArrayElement2dNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int idx1Value = varNode(idx1).lowerBound();
    const Int idx2Value = varNode(idx2).lowerBound();
    const Int expected =
        parVal(parMatrix.at(idx1Value - offsetIdx1).at(idx2Value - offsetIdx2));
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(ArrayElement2dNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_TRUE(invNode().replace(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  }
}

TEST_P(ArrayElement2dNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  VarNode outputNode = varNode(outputIdentifier);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual =
        parVal(parMatrix.at(varNode(idx1).lowerBound() - offsetIdx1)
                   .at(varNode(idx2).lowerBound() - offsetIdx2));
    EXPECT_EQ(expected, actual);
    return;
  }

  EXPECT_NE(varId(outputIdentifier), propagation::NULL_ID);
  const propagation::VarId outputId = varId(outputIdentifier);

  std::vector<propagation::VarId> inputVars;
  for (const auto& varNodeId : std::array<VarNodeId, 2>{idx1, idx2}) {
    EXPECT_NE(varId(varNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(varNodeId));
  }

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);
  EXPECT_FALSE(inputVals.empty());

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int row = inputVals.at(0) - offsetIdx1;
    const Int col = inputVals.at(1) - offsetIdx2;

    EXPECT_EQ(actual, parVal(parMatrix.at(row).at(col)));
  }
}

INSTANTIATE_TEST_CASE_P(ArrayElement2dNodeTest, ArrayElement2dNodeTestFixture,
                        ::testing::Values(0, 1, 2, 3, 4, 5, 6, 7));

}  // namespace atlantis::testing
