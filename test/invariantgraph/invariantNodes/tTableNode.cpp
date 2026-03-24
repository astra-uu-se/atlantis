#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/tableNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class TableNodeTestFixture : public NodeTestBase<TableNode> {
 public:
  std::string inputVar{"input_col"};
  std::vector<std::string> outputVars{"output_col_1", "output_col_2"};
  std::vector<std::vector<Int>> table{{0, 1, 2},
  {1, 2, 3},
    {2, 3, 4}};

  [[nodiscard]] size_t inputColIndex() const { return _paramData.data; }

  std::vector<Int> computeOutputs(bool isRegistered = false) {
    std::vector<Int> outputs;
    outputs.reserve(outputVars.size());
    EXPECT_GE(inputColIndex(), 0);
    EXPECT_LE(inputColIndex(), static_cast<Int>(outputVars.size()));
    if (isRegistered) {
      EXPECT_TRUE(varNode(inputVar).isFixed() ||
                  varId(inputVar) != propagation::NULL_ID);
      for (size_t i = 0; i < table.front().size(); ++i) {
        if (i == inputColIndex()) {
          continue;
        }
        outputs.emplace_back(table.at((varNode(inputVar).isFixed()
                                     ? varNode(inputVar).lowerBound()
                                     : _solver->currentValue(varId(inputVar)))).at(i));
      }
    } else {
      EXPECT_TRUE(varNode(inputVar).isFixed());
      for (size_t i = 0; i < table.front().size(); ++i) {
        if (i == inputColIndex()) {
          continue;
        }
        outputs.emplace_back(table.at(varNode(inputVar).lowerBound()).at(i));
      }
    }
    return outputs;
  }

  Int colLb(size_t i) const {
    Int lb = table.front().at(i);
    for (size_t j = 1; j < table.size(); ++j) {
      lb = std::min(lb, table.at(j).at(i));
    }
    return lb;
  }

  Int colUb(size_t i) const {
    Int ub = table.front().at(i);
    for (size_t j = 1; j < table.size(); ++j) {
      ub = std::max(ub, table.at(j).at(i));
    }
    return ub;
  }

  void SetUp() {
    NodeTestBase::SetUp();

    retrieveIntVarNode(
      colLb(inputColIndex()),
      shouldBeSubsumed()
          ? colLb(inputColIndex())
          : colUb(inputColIndex()),
      inputVar);

    for (size_t i = 0; i < outputVars.size(); ++i) {
      retrieveIntVarNode(-10, 10, outputVars.at(i));
    }
    createInvariantNode(*_invariantGraph,
                        varNodeIds(outputVars), varNodeId(inputVar), std::vector<std::vector<Int>>{table}, inputColIndex());
  }
};

TEST_P(TableNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().front(), varNodeId(inputVar));
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 2);
  EXPECT_THAT(invNode().outputVarNodeIds(), ::testing::ContainerEq(varNodeIds(outputVars)));
}

TEST_P(TableNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    for (const auto& outputVar : outputVars) {
      EXPECT_TRUE(varNode(outputVar).isFixed());
    }
    const auto expected = computeOutputs();
    std::vector<Int> actual;
    for (const auto& outputVar : outputVars) {
      actual.emplace_back(varNode(outputVar).lowerBound());
    }
    EXPECT_THAT(expected, ::testing::ContainerEq(actual));
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    for (const auto& outputVar : outputVars) {
      EXPECT_FALSE(varNode(outputVar).isFixed());
    }
  }
}

TEST_P(TableNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  const auto expected = computeOutputs(true);
  for (size_t i = 0; i < outputVars.size(); ++i) {
    VarNode& outputNode = varNode(outputVars.at(i));
    if (outputNode.isFixed()) {
      EXPECT_EQ(expected.at(i), varNode(outputVars.at(i)).lowerBound());
    }
  }

  const propagation::VarViewId inputVarId = varId(inputVar);
  EXPECT_NE(inputVarId, propagation::NULL_ID);

  for (const auto& outputVar : outputVars) {
    const propagation::VarViewId outputId = varId(outputVar);
    EXPECT_NE(outputId, propagation::NULL_ID);
  }

  for (Int inputVal = _solver->lowerBound(inputVarId);
       inputVal <= _solver->upperBound(inputVarId); ++inputVal) {
    _solver->beginMove();
    _solver->setValue(inputVarId, inputVal);
    _solver->endMove();

    _solver->beginProbe();
    for (const auto& outputVar : outputVars) {
      _solver->query(varId(outputVar));
    }
    _solver->endProbe();

    std::vector<Int> actual(outputVars.size());
    for (size_t i = 0; i < outputVars.size(); ++i) {
      actual.at(i) = (_solver->currentValue(varId(outputVars.at(i))));
    }

    EXPECT_EQ(actual, computeOutputs(true));
  }
}

INSTANTIATE_TEST_CASE_P(
    TableNodeTest, TableNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{1}, ParamData{2},
      ParamData{InvariantNodeAction::SUBSUME, 0}, ParamData{InvariantNodeAction::SUBSUME, 1}, ParamData{InvariantNodeAction::SUBSUME, 2}));

}  // namespace atlantis::testing
