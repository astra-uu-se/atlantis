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

  VarNodeId idx1VarNodeId{NULL_NODE_ID};
  VarNodeId idx2VarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int offsetIdx1 = 1;
  Int offsetIdx2 = 1;

  bool intParToBool(const Int val) { return std::abs(val) % 2 == 0; }

  Int parVal(const Int val) {
    return isIntElement() ? val : intParToBool(val) ? 0 : 1;
  }

  bool isIntElement() const { return _paramData.data <= 1; }
  bool idx1ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 0 || _paramData.data == 2);
  }
  bool idx2ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 1 || _paramData.data == 3);
  }

  Int computeOutput() {
    const Int row = varNode(idx1VarNodeId).lowerBound() - offsetIdx1;
    const Int col = varNode(idx2VarNodeId).lowerBound() - offsetIdx2;
    return parVal(parMatrix.at(row).at(col));
  }

  Int computeOutput(propagation::Solver& solver) {
    const Int row = (varNode(idx1VarNodeId).isFixed()
                         ? varNode(idx1VarNodeId).lowerBound()
                         : solver.currentValue(varId(idx1VarNodeId))) -
                    offsetIdx1;
    const Int col = (varNode(idx2VarNodeId).isFixed()
                         ? varNode(idx2VarNodeId).lowerBound()
                         : solver.currentValue(varId(idx2VarNodeId))) -
                    offsetIdx2;
    return parVal(parMatrix.at(row).at(col));
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    idx1VarNodeId = retrieveIntVarNode(
        offsetIdx1,
        shouldBeSubsumed() || idx1ShouldBeReplaced()
            ? offsetIdx1
            : (offsetIdx1 + static_cast<Int>(parMatrix.size()) - 1),
        "idx1");
    idx2VarNodeId = retrieveIntVarNode(
        offsetIdx2,
        shouldBeSubsumed() || idx2ShouldBeReplaced()
            ? offsetIdx2
            : (offsetIdx1 + static_cast<Int>(parMatrix.front().size()) - 1),
        "idx2");

    if (isIntElement()) {
      // int version of element
      outputVarNodeId = retrieveIntVarNode(-2, 1, outputIdentifier);
      createInvariantNode(idx1VarNodeId, idx2VarNodeId,
                          std::vector<std::vector<Int>>{parMatrix},
                          outputVarNodeId, offsetIdx1, offsetIdx2);
    } else {
      // bool version of element
      outputVarNodeId = retrieveBoolVarNode(outputIdentifier);
      std::vector<std::vector<bool>> boolMatrix;
      boolMatrix.reserve(parMatrix.size());
      for (const auto& row : parMatrix) {
        boolMatrix.emplace_back();
        for (const auto val : row) {
          boolMatrix.back().emplace_back(intParToBool(val));
        }
      }
      createInvariantNode(idx1VarNodeId, idx2VarNodeId, std::move(boolMatrix),
                          outputVarNodeId, offsetIdx1, offsetIdx2);
    }
  }
};

TEST_P(ArrayElement2dNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1VarNodeId);
  EXPECT_EQ(invNode().idx2(), idx2VarNodeId);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);

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
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
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
        parVal(parMatrix.at(varNode(idx1VarNodeId).lowerBound() - offsetIdx1)
                   .at(varNode(idx2VarNodeId).lowerBound() - offsetIdx2));
    EXPECT_EQ(expected, actual);
    return;
  }

  EXPECT_NE(varId(outputIdentifier), propagation::NULL_ID);
  const propagation::VarId outputId = varId(outputIdentifier);

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& idxVarNodeId :
       std::array<VarNodeId, 2>{idx1VarNodeId, idx2VarNodeId}) {
    if (!varNode(idxVarNodeId).isFixed()) {
      EXPECT_NE(varId(idxVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(idxVarNodeId));
    }
  }

  std::vector<Int> inputVals = makeInputVals(solver, inputVarIds);
  EXPECT_FALSE(inputVals.empty());

  while (increaseNextVal(solver, inputVarIds, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVarIds, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    expectVarVals(solver, inputVarIds, inputVals);

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(solver);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayElement2dNodeTest, ArrayElement2dNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::REPLACE, 0},
                      ParamData{InvariantNodeAction::REPLACE, 1}, ParamData{2},
                      ParamData{InvariantNodeAction::SUBSUME, 2},
                      ParamData{InvariantNodeAction::REPLACE, 2},
                      ParamData{InvariantNodeAction::REPLACE, 3}));

}  // namespace atlantis::testing
