#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class VarIntCountNodeTestFixture : public NodeTestBase<VarIntCountNode> {
 public:
  Int numInputs = 3;
  std::vector<std::string> inputVars;
  std::string needleVar{"needle"};
  std::string outputVar{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int needleVal = varNode(needleVar).isFixed()
                                ? varNode(needleVar).lowerBound()
                                : _solver->currentValue(varId(needleVar));
      Int occurrences = 0;
      for (const auto& var : inputVars) {
        const VarNode& inputVarNode = varNode(var);
        if (!inputVarNode.inDomain(needleVal)) {
          continue;
        }
        if (inputVarNode.isFixed() || varId(var) == propagation::NULL_ID) {
          EXPECT_TRUE(inputVarNode.isFixed());
          EXPECT_TRUE(varNode(var).inDomain(needleVal));
          ++occurrences;
        } else {
          occurrences += _solver->currentValue(varId(var)) == needleVal ? 1 : 0;
        }
      }
      return occurrences;
    }
    const Int needleVal = varNode(needleVar).lowerBound();
    Int occurrences = 0;
    for (const auto& var : inputVars) {
      EXPECT_TRUE(varNode(var).isFixed() || !varNode(var).inDomain(needleVal));

      occurrences +=
          varNode(var).isFixed() && varNode(var).inDomain(needleVal) ? 1 : 0;
    }
    return occurrences;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    inputVars.reserve(3);
    inputVars = {"input_0", "input_1", "input_2"};
    retrieveIntVarNode(2, 5, inputVars.at(0));
    retrieveIntVarNode(3, 5, inputVars.at(1));
    retrieveIntVarNode(4, 5, inputVars.at(2));
    if (shouldBeReplaced()) {
      retrieveIntVarNode(2, 2, needleVar);
    } else {
      retrieveIntVarNode(2, 5, needleVar);
    }

    retrieveIntVarNode(0, 2, outputVar);

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

TEST_P(VarIntCountNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
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

TEST_P(VarIntCountNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeReplaced()) {
    EXPECT_TRUE(varNode(needleVar).isFixed());
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

INSTANTIATE_TEST_CASE_P(
    VarIntCountNodeTest, VarIntCountNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
