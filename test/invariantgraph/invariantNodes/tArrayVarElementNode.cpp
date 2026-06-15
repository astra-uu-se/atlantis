#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElementNodeTestFixture
    : public NodeTestBase<ArrayVarElementNode> {
 protected:
  Var idxVar{"idx", std::vector<Int>{}, true};
  std::vector<Var> varArray;
  Var outputVar{"output", std::vector<Int>{}, true};

  Int offsetIdx = 1;

  [[nodiscard]] bool isIntElement() const { return _paramData.data == 0; }

  void SetUp() override {
    NodeTestBase::SetUp();

    if (isIntElement()) {
      varArray =
          std::vector<Var>{Var{"x1", -2, 0, true}, Var{"x2", -1, 1, true},
                           Var{"x3", 0, 2, true}};
      outputVar.domain = std::pair<Int, Int>{-2, 2};
      outputVar.isIntVar = true;
    } else {
      varArray =
          std::vector<Var>{Var{"x1", 0, 1, false}, Var{"x2", 0, 1, false},
                           Var{"x3", 0, 1, false}};
      outputVar.domain = std::pair<Int, Int>{0, 1};
      outputVar.isIntVar = false;
    }

    if (shouldBeReplaced()) {
      idxVar.domain = std::vector<Int>{offsetIdx};
    } else {
      idxVar.domain = std::pair<Int, Int>{
          offsetIdx, static_cast<Int>(varArray.size()) + offsetIdx - 1};
    }
    retrieveIntVarNode(idxVar);

    for (const auto& var : varArray) {
      if (isIntElement()) {
        retrieveIntVarNode(var);
      } else {
        retrieveBoolVarNode(var);
      }
    }

    if (isIntElement()) {
      retrieveIntVarNode(outputVar);
    } else {
      retrieveBoolVarNode(outputVar);
    }

    createInvariantNode(*_invariantGraph, varNodeId(idxVar),
                        varNodeIds(varArray), varNodeId(outputVar), offsetIdx);
  }
};

TEST_P(ArrayVarElementNodeTestFixture, replace) {
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

TEST_P(ArrayVarElementNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeReplaced()) {
    EXPECT_TRUE(varNode(idxVar).isFixed());
    EXPECT_FALSE(varNode(outputVar).isFixed());
    return;
  }

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarViewId> inputVarIds;
  std::vector<Int> inputVals;

  inputVarIds.emplace_back(varNode(idxVar).isFixed() ? propagation::NULL_ID
                                                     : varId(idxVar));
  inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                             ? varNode(idxVar).lowerBound()
                             : _solver->lowerBound(inputVarIds.back()));

  for (const auto& var : varArray) {
    inputVarIds.emplace_back(varNode(var).isFixed() ? propagation::NULL_ID
                                                    : varId(var));
    inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                               ? varNode(var).lowerBound()
                               : _solver->lowerBound(inputVarIds.back()));
  }

  EXPECT_EQ(inputVarIds.size(), inputVals.size());

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int actual = _solver->currentValue(outputId);

    const Int index = inputVals.at(0) - offsetIdx + 1;
    const Int expected = inputVals.at(index);

    EXPECT_EQ(actual, expected);
  }
}

TEST(ArrayVarElementNodeRegression, FixedBoolOutputPrunesIncompatibleIndices) {
  InvariantGraph graph;
  graph.open();

  const auto idx = graph.retrieveIntVarNode(
      std::make_shared<SearchDomain>(std::vector<Int>{1, 2}));
  const auto x1 = graph.retrieveBoolVarNode(false, false);
  const auto x2 = graph.retrieveBoolVarNode(true, false);
  const auto output = graph.retrieveBoolVarNode(true, false);

  const auto nodeId =
      graph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
          graph, idx, std::vector<VarNodeId>{x1, x2}, output, 1));
  auto& node = dynamic_cast<ArrayVarElementNode&>(graph.invariantNode(nodeId));
  graph.constraintSolver().fixPoint();
  graph.updateDomains();
  EXPECT_NO_THROW(node.updateState());
  EXPECT_TRUE(graph.varNode(idx).isFixed());
  EXPECT_EQ(graph.varNode(idx).lowerBound(), 2);
}

TEST(ArrayVarElementNodeRegression, FixedIndexAndOutputPruneSelectedChild) {
  InvariantGraph graph;
  graph.open();

  const auto idx = graph.retrieveIntVarNode(
      std::make_shared<SearchDomain>(std::vector<Int>{1}));
  const auto x1 = graph.retrieveBoolVarNode();
  const auto x2 = graph.retrieveBoolVarNode();
  const auto output = graph.retrieveBoolVarNode(true, true);

  const auto nodeId =
      graph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
          graph, idx, std::vector<VarNodeId>{x1, x2}, output, 1));
  auto& node = dynamic_cast<ArrayVarElementNode&>(graph.invariantNode(nodeId));
  graph.constraintSolver().fixPoint();
  graph.updateDomains();
  EXPECT_NO_THROW(node.updateState());
  EXPECT_TRUE(graph.varNode(x1).isFixed());
  EXPECT_TRUE(graph.varNode(x1).inDomain(bool{true}));
}

TEST(ArrayVarElementNodeRegression, SameDynamicVars) {
  InvariantGraph graph;
  graph.open();

  const std::string x{"x"};
  const std::string output{"output"};
  const auto idx =
      graph.retrieveIntVarNode(std::make_shared<SearchDomain>(1, 3));
  const auto xId = graph.retrieveBoolVarNode(x);
  const auto outputId = graph.retrieveBoolVarNode(output);

  const auto nodeId =
      graph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
          graph, idx, std::vector<VarNodeId>{xId, xId, xId}, outputId, 1));
  auto& node = dynamic_cast<ArrayVarElementNode&>(graph.invariantNode(nodeId));
  graph.constraintSolver().fixPoint();
  graph.updateDomains();
  EXPECT_NO_THROW(node.updateState());
  EXPECT_TRUE(node.canBeReplaced());
  EXPECT_TRUE(node.replace());
  EXPECT_EQ(graph.varNodeId(x), graph.varNodeId(output));
}

INSTANTIATE_TEST_SUITE_P(
    ArrayVarElementNodeTest, ArrayVarElementNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{InvariantNodeAction::REPLACE, 0},
                      ParamData{1},
                      ParamData{InvariantNodeAction::REPLACE, 1}));

}  // namespace atlantis::testing
