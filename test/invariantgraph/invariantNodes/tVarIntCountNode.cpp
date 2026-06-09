#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/countNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class VarIntCountNodeTestFixture : public NodeTestBase<CountNode> {
 protected:
  std::vector<Var> inputVars;
  Var needleVar{"needle", std::vector<Int>{}, true};
  Var outputVar{"output", std::vector<Int>{}, true};

  [[nodiscard]] Int computeOutput(const bool isRegistered = false) const {
    if (isRegistered) {
      const Int needleVal = varNodeConst(needleVar).isFixed()
                                ? varNodeConst(needleVar).lowerBound()
                                : _solver->currentValue(varId(needleVar));
      Int occurrences = 0;
      for (const auto& var : inputVars) {
        const VarNode& inputVarNode = varNodeConst(var);
        if (!inputVarNode.inDomain(needleVal)) {
          continue;
        }
        if (inputVarNode.isFixed() || varId(var) == propagation::NULL_ID) {
          EXPECT_TRUE(inputVarNode.isFixed());
          EXPECT_TRUE(varNodeConst(var).inDomain(needleVal));
          ++occurrences;
        } else {
          occurrences += _solver->currentValue(varId(var)) == needleVal ? 1 : 0;
        }
      }
      return occurrences;
    }
    const Int needleVal = varNodeConst(needleVar).lowerBound();
    Int occurrences = 0;
    for (const auto& var : inputVars) {
      EXPECT_TRUE(varNodeConst(var).isFixed() ||
                  !varNodeConst(var).inDomain(needleVal));

      occurrences +=
          varNodeConst(var).isFixed() && varNodeConst(var).inDomain(needleVal)
              ? 1
              : 0;
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVars.reserve(3);
    inputVars.emplace_back("input_0", 2, 5, true);
    inputVars.emplace_back("input_1", 3, 5, true);
    inputVars.emplace_back("input_2", 4, 5, true);
    for (const auto& var : inputVars) {
      retrieveIntVarNode(var);
    }
    if (shouldBeMadeImplicit()) {
      needleVar.domain = std::pair<Int, Int>(3, 3);
      outputVar.domain = std::pair<Int, Int>(1, 1);
    } else {
      needleVar.domain = std::pair<Int, Int>(2, 5);
      outputVar.domain = std::pair<Int, Int>(0, 2);
    }

    retrieveIntVarNode(needleVar);
    retrieveIntVarNode(outputVar);

    createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                        varNodeId(needleVar), varNodeId(outputVar));
  }
};

TEST_P(VarIntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  std::vector<VarNodeId> expectedInputs{varNodeIds(inputVars)};
  expectedInputs.emplace_back(varNodeId(needleVar));
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  const std::vector<VarNodeId> expectedOutputs{varNodeId(outputVar)};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
  EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_P(VarIntCountNodeTestFixture, makeImplicit) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
  invNode().updateState();
  if (shouldBeMadeImplicit()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeMadeImplicit());
    EXPECT_TRUE(invNode().makeImplicit());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeMadeImplicit());
  }
}

TEST_P(VarIntCountNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeMadeImplicit()) {
    EXPECT_TRUE(varNode(needleVar).isFixed());
    EXPECT_TRUE(varNode(outputVar).isFixed());
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (varNode(var).isFixed()) {
      continue;
    }
    if (varNode(needleVar).isFixed()) {
      const Int needleVal = varNode(needleVar).lowerBound();
      if (!varNode(var).inDomain(needleVal)) {
        continue;
      }
    }
    EXPECT_NE(varId(var), propagation::NULL_ID);
    inputVarIds.emplace_back(varId(var));
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
    VarIntCountNodeTest, VarIntCountNodeTestFixture,
    ::testing::Values(ParamData{},
                      ParamData{InvariantNodeAction::MAKE_IMPLICIT}));

}  // namespace atlantis::testing
