#include <boost/spirit/home/classic/phoenix/primitives.hpp>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/tableInNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class TableInNodeTestFixture : public NodeTestBase<TableInNode> {
 public:
  std::vector<std::string> inputVars{"output_col_1", "output_col_2",
                                     "output_col_3", "fixed_col"};
  std::string reifiedVar{"reified"};

  std::vector<std::vector<Int>> intTable{
      {2, 1, 2, 10}, {1, 2, 3, 10}, {0, 3, 4, 10}, {2, 2, 4, 10}};
  std::vector<std::vector<bool>> boolTable{{false, true, true, false},
                                           {true, false, false, false},
                                           {false, false, false, false}};

  std::vector<std::vector<Int>> table{};

  [[nodiscard]] bool isIntTable() const {
    return _paramData.data < static_cast<Int>(inputVars.size());
  }

  [[nodiscard]] Int fixedColIndex() const {
    return _paramData.data % static_cast<Int>(inputVars.size());
  }

  [[nodiscard]] bool colIsFixed(const size_t col) const {
    EXPECT_LE(col, inputVars.size());
    std::unordered_set<Int> colVals;
    colVals.reserve(table.size());
    for (const auto& row : table) {
      colVals.insert(row.at(col));
    }
    return colVals.size() == 1;
  }

  bool isViolating(const bool isRegistered = false) {
    std::vector<Int> colVals;
    colVals.reserve(inputVars.size());
    for (const auto& inputVar : inputVars) {
      if (isRegistered) {
        EXPECT_TRUE(varNode(inputVar).isFixed() ||
                    varId(inputVar) != propagation::NULL_ID);
      } else {
        EXPECT_TRUE(varNode(inputVar).isFixed());
      }
      colVals.emplace_back(varNode(inputVar).isFixed()
                               ? varNode(inputVar).lowerBound()
                               : _solver->currentValue(varId(inputVar)));
    }
    for (const auto& row : table) {
      EXPECT_EQ(row.size(), colVals.size());
      bool found = true;
      for (size_t c = 0; c < colVals.size(); ++c) {
        if (colVals.at(c) != row.at(c)) {
          found = false;
          break;
        }
      }
      if (found) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] Int colLb(size_t i) const {
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

  [[nodiscard]] Int colUb(size_t i) const {
    EXPECT_TRUE(isIntTable());
    Int ub = intTable.front().at(i);
    for (size_t j = 1; j < intTable.size(); ++j) {
      ub = std::max(ub, intTable.at(j).at(i));
    }
    return ub;
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    for (size_t c = 0; c < inputVars.size(); ++c) {
      if (isIntTable()) {
        if (shouldBeSubsumed() && fixedColIndex() == static_cast<Int>(c)) {
          retrieveIntVarNode(colLb(c), colLb(c), inputVars.at(c));
        } else {
          retrieveIntVarNode(-1, 10, inputVars.at(c));
        }
      } else {
        if (shouldBeSubsumed() && fixedColIndex() == static_cast<Int>(c)) {
          retrieveBoolVarNode(colLb(c) == 0, inputVars.at(c));
        } else {
          retrieveBoolVarNode(inputVars.at(c));
        }
      }
    }
    if (!shouldBeMadeImplicit()) {
      for (size_t i = 0; i < inputVars.size(); ++i) {
        if (!shouldBeReplaced() || fixedColIndex() != static_cast<Int>(i)) {
          _invariantGraph->root().addSearchVarNode(varNodeId(inputVars.at(i)));
        }
      }
    }

    if (isIntTable()) {
      table = intTable;
      if (shouldBeReplaced()) {
        EXPECT_GE(fixedColIndex(), 0);
        EXPECT_LT(fixedColIndex(), inputVars.size());
        table.erase(table.begin() + fixedColIndex());
      }
    } else {
      if (shouldBeReplaced()) {
        EXPECT_GE(fixedColIndex(), 0);
        EXPECT_LT(fixedColIndex(), inputVars.size());
        boolTable.resize(2, std::vector<bool>(inputVars.size()));
        for (size_t r = 0; r < boolTable.size(); ++r) {
          for (size_t c = 0; c < boolTable.at(r).size(); ++c) {
            if (static_cast<Int>(c) < fixedColIndex()) {
              boolTable.at(r).at(c) = static_cast<Int>(c) % 2 == 0;
            } else {
              boolTable.at(r).at(c) = static_cast<Int>(r) % 2 == 0;
            }
          }
          boolTable.at(r).back() = false;
        }
      }
      table.resize(boolTable.size(), std::vector<Int>(inputVars.size()));
      for (size_t r = 0; r < boolTable.size(); ++r) {
        for (size_t c = 0; c < boolTable.at(r).size(); ++c) {
          table.at(r).at(c) = boolTable.at(r).at(c) ? 0 : 1;
        }
      }
    }
    if (isIntTable()) {
      if (isReified()) {
        retrieveBoolVarNode(reifiedVar);
        createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                            std::vector<std::vector<Int>>{table},
                            varNodeId(reifiedVar));
      } else {
        createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                            std::vector<std::vector<Int>>{table}, shouldHold());
      }
    } else {
      if (isReified()) {
        retrieveBoolVarNode(reifiedVar);

        createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                            std::vector<std::vector<bool>>{boolTable},
                            varNodeId(reifiedVar));
      } else {
        createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                            std::vector<std::vector<bool>>{boolTable},
                            shouldHold());
      }
    }
  }
};

TEST_P(TableInNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().outputVarNodeIds().size(), isReified() ? 1 : 0);

  EXPECT_THAT(invNode().staticInputVarNodeIds(),
              ::testing::ContainerEq(varNodeIds(inputVars)));
}

TEST_P(TableInNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    const bool expected = isViolating(true);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const Int actual = varNode(reifiedVar).lowerBound();
      EXPECT_EQ(expected, actual);
    } else if (shouldHold()) {
      EXPECT_FALSE(expected);
    } else {
      EXPECT_TRUE(expected);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reifiedVar).isFixed());
    }
  }
}

TEST_P(TableInNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeMadeImplicit()) {
    for (Int i = 0; i < static_cast<Int>(inputVars.size()); ++i) {
      if (varNode(inputVars.at(i)).isFixed()) {
        continue;
      }
      EXPECT_TRUE(std::ranges::contains(
          _solver->searchVars().begin(), _solver->searchVars().end(),
          static_cast<propagation::VarId>(varId(inputVars.at(i)))));
    }
    return;
  }

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain(false);
      EXPECT_EQ(expected, actual);
    }
    if (shouldHold()) {
      EXPECT_FALSE(expected);
    }
    if (shouldFail()) {
      EXPECT_TRUE(expected);
    }
    return;
  }
  if (shouldBeReplaced()) {
    for (Int i = 0; i < static_cast<Int>(inputVars.size()); ++i) {
      if (varNode(inputVars.at(i)).isFixed()) {
        continue;
      }
      EXPECT_NE(varId(inputVars.at(i)), propagation::NULL_ID);
    }
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& inputVar : inputVars) {
    if (!varNode(inputVar).isFixed()) {
      EXPECT_NE(varId(inputVar), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVar));
    }
  }

  EXPECT_FALSE(inputVarIds.empty());

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(violVarId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const bool actual = _solver->currentValue(violVarId) > 0;
    const bool expected = isViolating(true);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    TableInNodeTest, TableInNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::REIFIED},
                      ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE, 0},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE, 1},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE, 2},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE, 4},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE, 5},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, 0},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, 1},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, 2},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, 4},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, 5},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, 6},
                      ParamData{InvariantNodeAction::MAKE_IMPLICIT}));

}  // namespace atlantis::testing
