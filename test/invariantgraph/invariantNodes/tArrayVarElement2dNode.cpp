#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElement2dNodeTestFixture
    : public NodeTestBase<ArrayVarElement2dNode> {
 protected:
  std::vector<std::vector<Var>> varMatrix;

  Var rowIdx{"rowIdx", std::vector<Int>{}, true};
  Var colIdx{"colIdx", std::vector<Int>{}, true};
  Var outputVar{"output", std::vector<Int>{}, true};

  Int rowOffset = -5;
  Int colOffset = 5;

  [[nodiscard]] bool isIntElement() const { return _paramData.data <= 2; }

  [[nodiscard]] bool rowIdxShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 0 || _paramData.data == 2);
  }

  [[nodiscard]] bool colIdxShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 1 || _paramData.data == 3);
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    if (isIntElement()) {
      varMatrix = std::vector<std::vector<Var>>{
          {Var{"x00", -2, 1, true}, Var{"x01", -1, 0, true}},
          {Var{"x10", 0, 1, true}, Var{"x11", 1, 2, true}}};
    } else {
      varMatrix = std::vector<std::vector<Var>>{
          {Var{"x00", 0, 1, false}, Var{"x01", 0, 1, false}},
          {Var{"x10", 0, 1, false}, Var{"x11", 0, 1, false}}};
    }

    rowIdx.domain = std::pair<Int, Int>{
        rowOffset, rowIdxShouldBeReplaced()
                       ? rowOffset
                       : (rowOffset + static_cast<Int>(varMatrix.size()) - 1)};
    retrieveIntVarNode(rowIdx);

    colIdx.domain = std::pair<Int, Int>{
        colOffset,
        colIdxShouldBeReplaced()
            ? colOffset
            : (colOffset + static_cast<Int>(varMatrix.front().size()) - 1)};
    retrieveIntVarNode(colIdx);

    for (const auto& row : varMatrix) {
      for (const auto& v : row) {
        if (isIntElement()) {
          retrieveIntVarNode(v);
        } else {
          retrieveBoolVarNode(v);
        }
      }
    }

    outputVar.isIntVar = isIntElement();
    if (isIntElement()) {
      outputVar.domain = std::pair<Int, Int>(-2, 2);
      retrieveIntVarNode(outputVar);
    } else {
      outputVar.domain = std::pair<Int, Int>(0, 1);
      retrieveBoolVarNode(outputVar);
    }

    createInvariantNode(*_invariantGraph, varNodeId(rowIdx), varNodeId(colIdx),
                        varNodeIds(varMatrix), varNodeId(outputVar), rowOffset,
                        colOffset);
  }
};

TEST_P(ArrayVarElement2dNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
  invNode().updateState();
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced());
    EXPECT_TRUE(invNode().replace());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced());
  }
}

TEST_P(ArrayVarElement2dNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarViewId> inputVarIds;
  std::vector<Int> inputVals;

  for (const auto& idx :
       std::array<std::string, 2>{rowIdx.identifier, colIdx.identifier}) {
    inputVarIds.emplace_back(varNode(idx).isFixed() ? propagation::NULL_ID
                                                    : varId(idx));
    inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                               ? varNode(idx).lowerBound()
                               : _solver->lowerBound(inputVarIds.back()));
  }
  for (const auto& row : varMatrix) {
    for (const auto& nId : row) {
      inputVarIds.emplace_back(varNode(nId).isFixed() ? propagation::NULL_ID
                                                      : varId(nId));
      inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                                 ? varNode(nId).lowerBound()
                                 : _solver->lowerBound(inputVarIds.back()));
    }
  }

  EXPECT_EQ(inputVarIds.size(), inputVals.size());

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    const Int actual = _solver->currentValue(outputId);
    const Int row = inputVals.at(0) - rowOffset;
    const Int col = inputVals.at(1) - colOffset;

    const Int index =
        2 + (row * static_cast<Int>(varMatrix.front().size()) + col);

    EXPECT_EQ(actual, inputVals.at(index));
  }
}

INSTANTIATE_TEST_SUITE_P(
    ArrayVarElement2dNodeTest, ArrayVarElement2dNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{InvariantNodeAction::REPLACE, 0},
                      ParamData{InvariantNodeAction::REPLACE, 1}, ParamData{2},
                      ParamData{InvariantNodeAction::REPLACE, 2},
                      ParamData{InvariantNodeAction::REPLACE, 3}));

TEST(ArrayVarElement2dNodeRegression, ReplaceHandlesReducedMatrixOffsets) {
  auto graph = std::make_shared<InvariantGraph>();
  graph->open();

  const auto rowIdx =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(1, 2), "row");
  const auto colIdx =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(5, 5), "col");
  const auto output =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(0, 9), "out");

  std::vector<VarNodeId> flat;
  flat.reserve(10);
  for (Int r = 0; r < 2; ++r) {
    for (Int c = 0; c < 5; ++c) {
      flat.emplace_back(graph->retrieveIntVarNode(r * 10 + c, r * 10 + c));
    }
  }

  const auto invId =
      graph->addInvariantNode(std::make_shared<ArrayVarElement2dNode>(
          *graph, rowIdx, colIdx, std::move(flat), output, 2, 1, 1));
  auto& node =
      dynamic_cast<ArrayVarElement2dNode&>(graph->invariantNode(invId));

  node.updateState();
  EXPECT_TRUE(node.canBeReplaced());
  EXPECT_TRUE(node.replace());
}

TEST(ArrayVarElement2dNodeRegression, ReplaceUniformInputMatrix) {
  auto graph = std::make_shared<FznInvariantGraph>();
  graph->open();

  const auto input =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(-5, 5));
  const auto rowIdx =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(0, 9));
  const auto colIdx =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(0, 9));
  const auto output =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(-5, 5));

  std::vector<std::vector<VarNodeId>> varMatrix(
      10, std::vector<VarNodeId>(10, input));

  const auto invId =
      graph->addInvariantNode(std::make_shared<ArrayVarElement2dNode>(
          *graph, rowIdx, colIdx, std::move(varMatrix), output, 0, 0));
  const auto& node =
      dynamic_cast<ArrayVarElement2dNode&>(graph->invariantNode(invId));
  EXPECT_TRUE(node.canBeReplaced());
  EXPECT_EQ(node.state(), InvariantNodeState::ACTIVE);
  graph->close();

  EXPECT_EQ(node.state(), InvariantNodeState::SUBSUMED);
}

}  // namespace atlantis::testing
