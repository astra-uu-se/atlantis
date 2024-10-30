#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class VarIntCountNodeTestFixture : public NodeTestBase<VarIntCountNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers{"input_1", "input_2", "input_3"};
  VarNodeId needleVarNodeId{NULL_NODE_ID};
  std::string needleIdentifier{"needle"};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int needleVal =
          varNode(needleIdentifier).isFixed()
              ? varNode(needleIdentifier).lowerBound()
              : _solver->currentValue(varId(needleIdentifier));
      Int occurrences = 0;
      for (const auto& identifier : inputIdentifiers) {
        const VarNode& inputVarNode = varNode(identifier);
        if (!inputVarNode.inDomain(needleVal)) {
          continue;
        }
        if (inputVarNode.isFixed() ||
            varId(identifier) == propagation::NULL_ID) {
          EXPECT_TRUE(inputVarNode.isFixed());
          EXPECT_TRUE(varNode(identifier).inDomain(needleVal));
          ++occurrences;
        } else {
          occurrences +=
              _solver->currentValue(varId(identifier)) == needleVal ? 1 : 0;
        }
      }
      return occurrences;
    }
    const Int needleVal = varNode(needleIdentifier).lowerBound();
    Int occurrences = 0;
    for (const auto& identifier : inputIdentifiers) {
      EXPECT_TRUE(varNode(identifier).isFixed() ||
                  !varNode(identifier).inDomain(needleVal));

      occurrences += varNode(identifier).isFixed() &&
                             varNode(identifier).inDomain(needleVal)
                         ? 1
                         : 0;
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds = {retrieveIntVarNode(2, 5, inputIdentifiers.at(0)),
                       retrieveIntVarNode(3, 5, inputIdentifiers.at(1)),
                       retrieveIntVarNode(4, 5, inputIdentifiers.at(2))};
    if (shouldBeReplaced()) {
      needleVarNodeId = retrieveIntVarNode(2, 2, needleIdentifier);
    } else {
      needleVarNodeId = retrieveIntVarNode(2, 5, needleIdentifier);
    }

    outputVarNodeId = retrieveIntVarNode(0, 2, outputIdentifier);

    createInvariantNode(*_invariantGraph,
                        std::vector<VarNodeId>{inputVarNodeIds},
                        needleVarNodeId, outputVarNodeId);
  }
};

TEST_P(VarIntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  std::vector<VarNodeId> expectedInputs{inputVarNodeIds};
  expectedInputs.emplace_back(needleVarNodeId);
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  std::vector<VarNodeId> expectedOutputs{outputVarNodeId};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
  EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_P(VarIntCountNodeTestFixture, application) {
  _solver->open();
  addInputVarsToSolver();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_EQ(invNode().violationVarId(), propagation::NULL_ID);
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  // x1, x2, x3, and needleVar
  EXPECT_EQ(_solver->searchVars().size(), 4);
  // x1, x2, x3, needleVar, and (outputVarNodeId or intermediate)
  EXPECT_EQ(_solver->numVars(), 5);

  // countEq
  EXPECT_EQ(_solver->numInvariants(), 1);

  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(_solver->lowerBound(varId(outputVarNodeId)), 0);
    EXPECT_GT(_solver->upperBound(varId(outputVarNodeId)), 0);
  }
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
    EXPECT_TRUE(varNode(needleIdentifier).isFixed());
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& identifier : inputIdentifiers) {
    if (varNode(identifier).isFixed()) {
      continue;
    }
    if (varNode(needleIdentifier).isFixed()) {
      const Int needleVar = varNode(needleIdentifier).lowerBound();
      if (!varNode(identifier).inDomain(needleVar)) {
        continue;
      }
    }
    EXPECT_NE(varId(identifier), propagation::NULL_ID);
    inputVarIds.emplace_back(varId(identifier));
  }

  const propagation::VarViewId outputId = varId(outputIdentifier);
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
