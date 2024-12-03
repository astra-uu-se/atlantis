#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class IntCountNodeTestFixture : public NodeTestBase<IntCountNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers{"input_1", "input_2", "input_3"};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};
  Int needle{2};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int occurrences = 0;
      for (const auto& identifier : inputIdentifiers) {
        if (!varNode(identifier).inDomain(needle)) {
          continue;
        }
        if (varNode(identifier).isFixed() ||
            varId(identifier) == propagation::NULL_ID) {
          EXPECT_TRUE(varNode(identifier).inDomain(needle));
          ++occurrences;
        } else {
          occurrences +=
              _solver->currentValue(varId(identifier)) == needle ? 1 : 0;
        }
      }
      return occurrences;
    }
    Int occurrences = 0;
    for (const auto& identifier : inputIdentifiers) {
      occurrences +=
          varNode(identifier).isFixed() && varNode(identifier).inDomain(needle)
              ? 1
              : 0;
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        inputVarNodeIds = {
            retrieveIntVarNode(0, 1, inputIdentifiers.at(0)),
            retrieveIntVarNode(std::vector<Int>{1, 3, 4, 5, 6, 7, 8, 9, 10},
                               inputIdentifiers.at(1)),
            retrieveIntVarNode(std::vector<Int>{2}, inputIdentifiers.at(2))};
        outputVarNodeId = retrieveIntVarNode(0, 3, outputIdentifier);
      } else {
        inputVarNodeIds = {retrieveIntVarNode(2, 2, inputIdentifiers.at(0)),
                           retrieveIntVarNode(1, 10, inputIdentifiers.at(1)),
                           retrieveIntVarNode(1, 10, inputIdentifiers.at(2))};
        outputVarNodeId = retrieveIntVarNode(0, 1, outputIdentifier);
      }
    } else {
      if (_paramData.data == 0) {
        inputVarNodeIds = {
            retrieveIntVarNode(1, 3, inputIdentifiers.at(0)),
            retrieveIntVarNode(std::vector<Int>{1, 3, 4, 5, 6, 7, 8, 9, 10},
                               inputIdentifiers.at(1)),
            retrieveIntVarNode(std::vector<Int>{2}, inputIdentifiers.at(2))};
        outputVarNodeId = retrieveIntVarNode(1, 2, outputIdentifier);
      } else {
        inputVarNodeIds = {retrieveIntVarNode(1, 10, inputIdentifiers.at(0)),
                           retrieveIntVarNode(1, 10, inputIdentifiers.at(1)),
                           retrieveIntVarNode(1, 10, inputIdentifiers.at(2))};
        outputVarNodeId = retrieveIntVarNode(0, 3, outputIdentifier);
      }
    }

    createInvariantNode(*_invariantGraph,
                        std::vector<VarNodeId>{inputVarNodeIds}, needle,
                        outputVarNodeId);
  }
};

TEST_P(IntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());

  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputVarNodeIds);
  EXPECT_THAT(inputVarNodeIds, ContainerEq(invNode().staticInputVarNodeIds()));

  std::vector<VarNodeId> expectedOutputs{outputVarNodeId};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
  EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_P(IntCountNodeTestFixture, application) {
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

  EXPECT_EQ(_solver->searchVars().size(), 3);
  EXPECT_EQ(_solver->numVars(), 4);

  EXPECT_EQ(_solver->numInvariants(), 1);

  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(_solver->lowerBound(varId(outputVarNodeId)), 0);
    EXPECT_GT(_solver->upperBound(varId(outputVarNodeId)), 0);
  }
}

TEST_P(IntCountNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    // disabled for the MZN challange. this should be computed by Gecode.
    // EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);

    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual = varNode(outputVarNodeId).lowerBound();
    // disabled for the MZN challange. this should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(IntCountNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& identifier : inputIdentifiers) {
    if (!varNode(identifier).isFixed() &&
        varNode(identifier).inDomain(needle)) {
      EXPECT_NE(varId(identifier), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifier));
    }
  }

  // disabled for the MZN challange. this should be computed by Gecode.
  // EXPECT_EQ(inputVarIds.empty(), shouldBeSubsumed());

  if (shouldBeSubsumed()) {
    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual = varNode(outputIdentifier).lowerBound();
    // disabled for the MZN challange. this should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
    return;
  }

  VarNode& outputNode = varNode(outputIdentifier);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual = computeOutput(true);
    EXPECT_EQ(expected, actual);
    return;
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
    IntCountNodeTest, IntCountNodeTestFixture,
    ::testing::Values(ParamData{int{0}}, ParamData{int{1}},
                      ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::SUBSUME, 1}));

}  // namespace atlantis::testing
