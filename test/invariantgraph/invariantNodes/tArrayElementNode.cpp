#include <numeric>
#include <ranges>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayElementNodeTestFixture : public NodeTestBase<ArrayElementNode> {
 protected:
  Var idxVar{"idx", std::vector<Int>{}, true};
  Var outputVar{"output", std::vector<Int>{}, true};

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

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      idxVar.fixToValue(offsetIdx);
    } else {
      idxVar.domain = std::pair{offsetIdx, offsetIdx + static_cast<Int>(parArray.size()) - 1};
    }

    outputVar.isIntVar = isIntElement();

    retrieveIntVarNode(idxVar);

    if (isIntElement()) {
      // int version of element
      outputVar.domain = std::pair<Int,Int>{-2, 1};
      retrieveIntVarNode(outputVar);
      createInvariantNode(*_invariantGraph, std::vector<Int>{parArray},
                          varNodeId(idxVar), varNodeId(outputVar), offsetIdx);
    } else {
      // bool version of element
      outputVar.domain = std::vector<Int>{0, 1};
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

  EXPECT_EQ(invNode().staticInputVarNodeIds().front(), varNodeId(idxVar));
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
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
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
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

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

INSTANTIATE_TEST_SUITE_P(
    ArrayElementNodeTest, ArrayElementNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::REPLACE, 0}, ParamData{1},
                      ParamData{InvariantNodeAction::SUBSUME, 1},
                      ParamData{InvariantNodeAction::REPLACE, 1}));

TEST(ArrayElementNodeRegression, UpdateStatePrunesOutOfRangeIndexValues) {
  auto graph = std::make_shared<InvariantGraph>();
  graph->open();

  const auto idx =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(std::vector<Int>{
                                    std::numeric_limits<Int>::min(), 1}),
                                "idx");
  const auto output =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(-2, 1), "out");

  const auto invId = graph->addInvariantNode(std::make_shared<ArrayElementNode>(
      *graph, std::vector<Int>{-2, -1, 0, 1}, idx, output, 1));
  auto& node = dynamic_cast<ArrayElementNode&>(graph->invariantNode(invId));
  graph->constraintSolver().fixPoint();
  graph->updateDomains();

  EXPECT_NO_THROW(node.updateState());
  EXPECT_TRUE(graph->varNode(idx).isFixed());
  EXPECT_EQ(graph->varNode(idx).lowerBound(), 1);
  EXPECT_TRUE(graph->varNode(output).isFixed());
  EXPECT_EQ(graph->varNode(output).lowerBound(), -2);
}

}  // namespace atlantis::testing
