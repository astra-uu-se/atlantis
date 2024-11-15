#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class IntCountNodeTestFixture : public NodeTestBase<IntCountNode> {
 public:
  Int numInputs = 3;
  std::vector<Var> inputVars;
  Var outputVar{NULL_NODE_ID, "output"};

  Int needle{2};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int occurrences = 0;
      for (const auto& var : inputVars) {
        if (!varNode(var).inDomain(needle)) {
          continue;
        }
        if (varNode(var).isFixed() || varId(var) == propagation::NULL_ID) {
          EXPECT_TRUE(varNode(var).inDomain(needle));
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
          varNode(var).isFixed() && varNode(var).inDomain(needle) ? 1 : 0;
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    for (Int i = 0; i < numInputs; ++i) {
      inputVars.emplace_back(Var{NULL_NODE_ID, "input_" + std::to_string(i)});
    }
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        inputVars.at(0).id =
            retrieveIntVarNode(0, 1, inputVars.at(0).identifier);
        inputVars.at(1).id =
            retrieveIntVarNode(std::vector<Int>{1, 3, 4, 5, 6, 7, 8, 9, 10},
                               inputVars.at(1).identifier);
        inputVars.at(2).id =
            retrieveIntVarNode(std::vector<Int>{2}, inputVars.at(2).identifier);
        outputVar.id = retrieveIntVarNode(0, 3, outputVar.identifier);
      } else {
        inputVars.at(0).id =
            retrieveIntVarNode(2, 2, inputVars.at(0).identifier);
        inputVars.at(1).id =
            retrieveIntVarNode(1, 10, inputVars.at(1).identifier);
        inputVars.at(2).id =
            retrieveIntVarNode(1, 10, inputVars.at(2).identifier);
        outputVar.id = retrieveIntVarNode(0, 1, outputVar.identifier);
      }
    } else {
      if (_paramData.data == 0) {
        inputVars.at(0).id =
            retrieveIntVarNode(1, 3, inputVars.at(0).identifier);
        inputVars.at(1).id =
            retrieveIntVarNode(std::vector<Int>{1, 3, 4, 5, 6, 7, 8, 9, 10},
                               inputVars.at(1).identifier);
        inputVars.at(2).id =
            retrieveIntVarNode(std::vector<Int>{2}, inputVars.at(2).identifier);
        outputVar.id = retrieveIntVarNode(1, 2, outputVar.identifier);
      } else {
        inputVars.at(0).id =
            retrieveIntVarNode(1, 10, inputVars.at(0).identifier);
        inputVars.at(1).id =
            retrieveIntVarNode(1, 10, inputVars.at(1).identifier);
        inputVars.at(2).id =
            retrieveIntVarNode(1, 10, inputVars.at(2).identifier);
        outputVar.id = retrieveIntVarNode(0, 3, outputVar.identifier);
      }
    }
    for (size_t i = 0; i < inputVars.size(); ++i) {
      inputVars.at(i).identifier = "input_" + std::to_string(i);
    }

    createInvariantNode(*_invariantGraph, varNodeIds(inputVars), needle,
                        outputVar.id);
  }
};  // namespace atlantis::testing

TEST_P(IntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());

  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputVarNodeIds);
  EXPECT_THAT(inputVarNodeIds, ContainerEq(invNode().staticInputVarNodeIds()));

  const std::vector<VarNodeId> expectedOutputs{outputVarNodeId};

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
    /*
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);

    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual = varNode(outputVarNodeId).lowerBound();
    // disabled for the MZN challange. this should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(IntCountNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed() && varNode(var).inDomain(needle)) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  // disabled for the MZN challange. this should be computed by Gecode.
  // EXPECT_EQ(inputVarIds.empty(), shouldBeSubsumed());

  if (shouldBeSubsumed()) {
    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual = varNode(outputIdentifier).lowerBound();
    // disabled for the MZN challange. this should be computed by Gecode.
    /*
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
    */
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

INSTANTIATE_TEST_CASE_P(
    IntCountNodeTest, IntCountNodeTestFixture,
    ::testing::Values(ParamData{int{0}}, ParamData{int{1}},
                      ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::SUBSUME, 1}));

}  // namespace atlantis::testing
