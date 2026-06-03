#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class GlobalCardinalityNodeTestFixture
    : public NodeTestBase<GlobalCardinalityNode> {
 protected:
  std::vector<Var> inputVars;
  std::vector<Int> cover{2, 4};
  std::vector<Var> outputVars;

  std::vector<Int> computeOutputs(bool isRegistered = false) {
    std::vector<Int> outputVals(cover.size(), 0);
    if (isRegistered) {
      for (const auto& var : inputVars) {
        const Int value = varNode(var).isFixed()
                              ? varNode(var).lowerBound()
                              : _solver->currentValue(varId(var));
        for (size_t j = 0; j < cover.size(); ++j) {
          if (value == cover.at(j)) {
            outputVals.at(j)++;
          }
        }
      }
      return outputVals;
    }
    for (const auto& var : inputVars) {
      const Int value = varNode(var).lowerBound();
      for (size_t j = 0; j < cover.size(); ++j) {
        if (value == cover.at(j)) {
          outputVals.at(j)++;
        }
      }
    }
    return outputVals;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVars = std::vector<Var>{Var{"input_1", {}, true}, Var{"input_2", {}, true}};
    if (shouldBeSubsumed()) {
      inputVars.at(0).domain = std::pair<Int, Int>{0, 2};
      inputVars.at(1).domain = std::vector<Int>{1, 3, 5};
    } else if (shouldBeReplaced()) {
      inputVars.at(0).domain = std::pair<Int, Int>{1, 3};
      inputVars.at(1).domain = std::pair<Int, Int>{1, 3};
    } else {
      inputVars.at(0).domain = std::pair<Int, Int>{1, 5};
      inputVars.at(1).domain = std::pair<Int, Int>{1, 5};
    }
    for (const auto& var : inputVars) {
      retrieveIntVarNode(var);
    }

    outputVars.clear();
    for (size_t i = 0; i < cover.size(); ++i) {
      outputVars.emplace_back("output_" + std::to_string(i + 1), 0, static_cast<Int>(inputVars.size()), true);
      retrieveIntVarNode(outputVars.back());
    }

    createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                        std::vector<Int>{cover}, varNodeIds(outputVars));
  }
};

TEST_P(GlobalCardinalityNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    const std::vector<Int> expected = computeOutputs();
    std::vector<Int> actual;
    for (const auto& outputVarNodeId : outputVars) {
      actual.emplace_back(varNode(outputVarNodeId).lowerBound());
    }
    EXPECT_THAT(expected, ContainerEq(actual));
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    size_t numFixed = 0;
    for (const auto& outputVar : outputVars) {
      numFixed += varNode(outputVar).isFixed() ? 1 : 0;
    }
    EXPECT_LT(numFixed, outputVars.size());
  }
}

TEST_P(GlobalCardinalityNodeTestFixture, replace) {
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

TEST_P(GlobalCardinalityNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  for (size_t i = 0; i < cover.size(); ++i) {
    if (varNode(outputVars.at(i)).isFixed()) {
      const Int actual = varNode(outputVars.at(i)).lowerBound();
      Int expected = 0;
      for (const auto& var : inputVars) {
        expected += varNode(var).isFixed() && varNode(var).inDomain(cover.at(i))
                        ? 1
                        : 0;
      }
      EXPECT_EQ(expected, actual);
    }
  }

  std::vector<propagation::VarViewId> outputIds;
  for (const auto& var : outputVars) {
    outputIds.emplace_back(varNode(var).isFixed() ? propagation::NULL_ID
                                                  : varId(var));
  }
  bool allNull = true;
  for (const auto& outputId : outputIds) {
    allNull = allNull && outputId == propagation::NULL_ID;
  }

  EXPECT_EQ(allNull, shouldBeSubsumed());
  if (shouldBeSubsumed()) {
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  std::vector<Int> inputVals;

  for (const auto& var : inputVars) {
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
    for (const auto& outputId : outputIds) {
      if (outputId != propagation::NULL_ID) {
        _solver->query(outputId);
      }
    }
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const std::vector<Int> expected = computeOutputs(true);

    EXPECT_EQ(expected.size(), outputIds.size());
    for (size_t i = 0; i < expected.size(); ++i) {
      const Int actual = outputIds.at(i) == propagation::NULL_ID
                             ? varNode(outputVars.at(i)).lowerBound()
                             : _solver->currentValue(outputIds.at(i));
      EXPECT_EQ(actual, expected.at(i));
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    GlobalCardinalityNodeTest, GlobalCardinalityNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
