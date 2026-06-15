#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/countNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class IntCountNodeTestFixture : public NodeTestBase<CountNode> {
 protected:
  Int numInputs = 3;
  std::vector<Var> inputVars;
  Var outputVar{"output", std::vector<Int>{}, true};

  Int needle{2};

  [[nodiscard]] Int computeOutput(const bool isRegistered = false) const {
    if (isRegistered) {
      Int occurrences = 0;
      for (const auto& var : inputVars) {
        if (!varNodeConst(var).inDomain(needle)) {
          continue;
        }
        if (varNodeConst(var).isFixed() || varId(var) == propagation::NULL_ID) {
          EXPECT_TRUE(varNodeConst(var).inDomain(needle));
          ++occurrences;
        } else {
          occurrences += _solver->currentValue(varId(var)) == needle ? 1 : 0;
        }
      }
      return occurrences;
    }
    Int occurrences = 0;
    for (const auto& var : inputVars) {
      occurrences +=
          varNodeConst(var).isFixed() && varNodeConst(var).inDomain(needle) ? 1
                                                                            : 0;
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    for (Int i = 0; i < numInputs; ++i) {
      inputVars.emplace_back("input_" + std::to_string(i), std::vector<Int>{},
                             true);
    }
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        inputVars.at(0).domain = std::pair<Int, Int>{0, 1};
        inputVars.at(1).domain = std::vector<Int>{1, 3, 4, 5, 6, 7, 8, 9, 10};
        inputVars.at(2).domain = std::vector<Int>{2};

        outputVar.domain = std::pair<Int, Int>{0, 3};
      } else {
        inputVars.at(0).domain = std::pair<Int, Int>{2, 2};
        inputVars.at(1).domain = std::pair<Int, Int>{1, 10};
        inputVars.at(2).domain = std::pair<Int, Int>{1, 10};

        outputVar.domain = std::pair<Int, Int>{0, 1};
      }
    } else {
      if (_paramData.data == 0) {
        inputVars.at(0).domain = std::pair<Int, Int>{1, 3};
        inputVars.at(1).domain = std::vector<Int>{1, 3, 4, 5, 6, 7, 8, 9, 10};
        inputVars.at(2).domain = std::vector<Int>{2};

        outputVar.domain = std::pair<Int, Int>{1, 2};
      } else {
        inputVars.at(0).domain = std::pair<Int, Int>{1, 10};
        inputVars.at(1).domain = std::pair<Int, Int>{1, 10};
        inputVars.at(2).domain = std::pair<Int, Int>{1, 10};

        outputVar.domain = std::pair<Int, Int>{0, 3};
      }
    }
    for (const auto& var : inputVars) {
      retrieveIntVarNode(var);
    }
    retrieveIntVarNode(outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(outputVar),
                        varNodeIds(inputVars), needle);
  }
};  // namespace atlantis::testing

TEST_P(IntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  std::vector<VarNodeId> expectedInputs = varNodeIds(inputVars);

  EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  const std::vector<VarNodeId> expectedOutputs{varNodeId(outputVar)};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
  EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_P(IntCountNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
  invNode().updateState();
  if (shouldBeSubsumed()) {
    // disabled for the MZN challenge. this should be computed by Gecode.
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);

    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual = varNode(outputVar).lowerBound();
    // disabled for the MZN challenge. this should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(IntCountNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed() && varNode(var).inDomain(needle)) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  // disabled for the MZN challenge. this should be computed by Gecode.
  // EXPECT_EQ(inputVarIds.empty(), shouldBeSubsumed());

  if (shouldBeSubsumed()) {
    // disabled for the MZN challenge. this should be computed by Gecode.
    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual = varNode(outputVar).lowerBound();
    return;
  }

  VarNode& outputNode = varNode(outputVar);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual = computeOutput(true);
    EXPECT_EQ(expected, actual);
    return;
  }

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int actual = _solver->currentValue(outputId);
    const Int expected = computeOutput(true);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_SUITE_P(
    IntCountNodeTest, IntCountNodeTestFixture,
    ::testing::Values(ParamData{int{0}}, ParamData{int{1}},
                      ParamData{InvariantNodeAction::SUBSUME, 0}));

}  // namespace atlantis::testing
