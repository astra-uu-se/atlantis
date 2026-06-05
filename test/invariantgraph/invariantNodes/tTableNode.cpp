#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/invariantNodes/tableNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class TableNodeTestFixture : public NodeTestBase<TableNode> {
 protected:
  Var inputVar{"input_col", std::vector<Int>{}, true};
  std::vector<Var> outputVars{Var{"output_col_1", std::vector<Int>{}, true},
                              Var{"output_col_2", std::vector<Int>{}, true},
                              Var{"output_col_3", std::vector<Int>{}, true}};

  std::vector<std::vector<Int>> intTable{
      {0, 1, 2, 10}, {1, 2, 3, 10}, {2, 3, 4, 10}};
  std::vector<std::vector<bool>> boolTable{{false, true, false, false},
                                           {true, false, true, false}};

  std::vector<std::vector<Int>> table{};

  [[nodiscard]] bool isIntTable() const {
    return _paramData.data < static_cast<Int>(outputVars.size());
  }
  [[nodiscard]] size_t inputColIndex() const {
    return _paramData.data % static_cast<Int>(outputVars.size() + 1);
  }

  [[nodiscard]] bool colIsFixed(const size_t col) const {
    EXPECT_LE(col, outputVars.size());
    std::unordered_set<Int> colVals;
    colVals.reserve(table.size());
    for (const auto& row : table) {
      colVals.insert(row.at(col));
    }
    return colVals.size() == 1;
  }

  std::vector<Int> computeOutputs(const bool isRegistered = false) {
    std::vector<Int> outputs;
    outputs.reserve(outputVars.size());
    EXPECT_GE(inputColIndex(), 0);
    EXPECT_LE(inputColIndex(), static_cast<Int>(outputVars.size()));
    if (isRegistered) {
      EXPECT_TRUE(varNode(inputVar).isFixed() ||
                  varId(inputVar) != propagation::NULL_ID);
    } else {
      EXPECT_TRUE(varNode(inputVar).isFixed());
    }
    const Int inputColVal = varNode(inputVar).isFixed()
                                ? varNode(inputVar).lowerBound()
                                : _solver->currentValue(varId(inputVar));
    Int row = -1;
    for (Int r = 0; r < static_cast<Int>(table.size()); ++r) {
      if (table.at(r).at(inputColIndex()) == inputColVal) {
        row = r;
        break;
      }
    }
    for (size_t i = 0; i < table.front().size(); ++i) {
      if (i == inputColIndex()) {
        continue;
      }
      outputs.emplace_back(table.at(row).at(i));
    }
    return outputs;
  }

  [[nodiscard]] Int colLb(const size_t i) const {
    if (isIntTable()) {
      Int lb = intTable.front().at(i);
      for (size_t j = 1; j < intTable.size(); ++j) {
        lb = std::min(lb, intTable.at(j).at(i));
      }
      return lb;
    }
    Int lb = 0;
    for (size_t j = 1; j < boolTable.size(); ++j) {
      lb = std::min(lb, Int{boolTable.at(j).at(i) ? 0 : 1});
    }
    return lb;
  }

  [[nodiscard]] Int colUb(const size_t i) const {
    EXPECT_TRUE(isIntTable());
    Int ub = intTable.front().at(i);
    for (size_t j = 1; j < intTable.size(); ++j) {
      ub = std::max(ub, intTable.at(j).at(i));
    }
    return ub;
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    inputVar.isIntVar = isIntTable();

    if (isIntTable()) {
      inputVar.domain = std::pair<Int, Int>{
          colLb(inputColIndex()),
          shouldBeSubsumed() ? colLb(inputColIndex()) : colUb(inputColIndex())};
      retrieveIntVarNode(inputVar);
    } else {
      if (shouldBeSubsumed()) {
        inputVar.domain = std::vector<Int>{colLb(inputColIndex()) == 0};
        retrieveBoolVarNode(inputVar);
      } else {
        inputVar.domain = std::vector<Int>{0, 1};
        retrieveBoolVarNode(inputVar);
      }
    }

    for (auto& outputVar : outputVars) {
      outputVar.isIntVar = isIntTable();
      if (isIntTable()) {
        outputVar.domain = std::pair<Int, Int>{-10, 10};
        retrieveIntVarNode(outputVar);
      } else {
        outputVar.domain = std::pair<Int, Int>{0, 1};
        retrieveBoolVarNode(outputVar);
      }
    }

    if (isIntTable()) {
      table = intTable;
    } else {
      table.resize(boolTable.size(), std::vector<Int>(outputVars.size() + 1));
      for (size_t r = 0; r < boolTable.size(); ++r) {
        for (size_t c = 0; c < boolTable.at(r).size(); ++c) {
          table.at(r).at(c) = boolTable.at(r).at(c) ? 0 : 1;
        }
      }
    }

    if (!shouldBeMadeImplicit()) {
      _invariantGraph->root().addSearchVarNode(varNodeId(inputVar));
    }

    if (isIntTable()) {
      createInvariantNode(
          *_invariantGraph, varNodeIds(outputVars), varNodeId(inputVar),
          std::vector<std::vector<Int>>{table}, inputColIndex());
    } else {
      createInvariantNode(
          *_invariantGraph, varNodeIds(outputVars), varNodeId(inputVar),
          std::vector<std::vector<bool>>{boolTable}, inputColIndex());
    }
  }
};

TEST_P(TableNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().front(), varNodeId(inputVar));
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 3);
  EXPECT_THAT(invNode().outputVarNodeIds(),
              ::testing::ContainerEq(varNodeIds(outputVars)));
}

TEST_P(TableNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
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
    for (size_t c = 0; c <= outputVars.size(); ++c) {
      if (c == inputColIndex()) {
        continue;
      }
      const auto& outputVar = outputVars.at(c < inputColIndex() ? c : c - 1);
      if (colIsFixed(c)) {
        EXPECT_TRUE(varNode(outputVar).isFixed());
      } else {
        EXPECT_FALSE(varNode(outputVar).isFixed());
      }
    }
  }
}

TEST_P(TableNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeMadeImplicit()) {
    if (!varNode(inputVar).isFixed()) {
      EXPECT_TRUE(std::ranges::contains(
          _solver->searchVars().begin(), _solver->searchVars().end(),
          static_cast<propagation::VarId>(varId(inputVar))));
    }
    for (Int i = 0; i < static_cast<Int>(outputVars.size()); ++i) {
      if (varNode(outputVars.at(i)).isFixed()) {
        continue;
      }
      EXPECT_TRUE(std::ranges::contains(
          _solver->searchVars().begin(), _solver->searchVars().end(),
          static_cast<propagation::VarId>(varId(outputVars.at(i)))));
    }
    return;
  }

  const auto expected = computeOutputs(true);
  for (size_t i = 0; i < outputVars.size(); ++i) {
    VarNode& outputNode = varNode(outputVars.at(i));
    if (outputNode.isFixed()) {
      EXPECT_EQ(expected.at(i), varNode(outputVars.at(i)).lowerBound());
    }
  }

  const propagation::VarViewId inputVarId = varId(inputVar);
  if (shouldBeSubsumed()) {
    if (isIntTable()) {
      EXPECT_EQ(inputVarId, propagation::NULL_ID);
    } else {
      EXPECT_TRUE(varNode(inputVar).isFixed());
    }
    for (const auto& outputVar : outputVars) {
      EXPECT_TRUE(varNode(outputVar).isFixed());
    }
    return;
  }

  EXPECT_NE(inputVarId, propagation::NULL_ID);
  for (size_t c = 0; c <= outputVars.size(); ++c) {
    if (c == inputColIndex()) {
      continue;
    }
    const auto& outputVar = outputVars.at(c < inputColIndex() ? c : c - 1);
    if (colIsFixed(c)) {
      EXPECT_EQ(varId(outputVar), propagation::NULL_ID);
    } else {
      EXPECT_NE(varId(outputVar), propagation::NULL_ID);
    }
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
      if (varNode(outputVars.at(i)).isFixed()) {
        actual.at(i) = varNode(outputVars.at(i)).lowerBound();
      } else {
        actual.at(i) = _solver->currentValue(varId(outputVars.at(i)));
      }
    }

    EXPECT_EQ(actual, computeOutputs(true));
  }
}

INSTANTIATE_TEST_SUITE_P(
    TableNodeTest, TableNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{1}, ParamData{2}, ParamData{4},
                      ParamData{5}, ParamData{6},
                      ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::SUBSUME, 1},
                      ParamData{InvariantNodeAction::SUBSUME, 2},
                      ParamData{InvariantNodeAction::SUBSUME, 4},
                      ParamData{InvariantNodeAction::SUBSUME, 5},
                      ParamData{InvariantNodeAction::SUBSUME, 6},
                      ParamData{InvariantNodeAction::MAKE_IMPLICIT}));

}  // namespace atlantis::testing
