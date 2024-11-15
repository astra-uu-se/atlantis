#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::Contains;

class IntPlusNodeTestFixture : public NodeTestBase<IntPlusNode> {
 public:
  std::vector<Var> inputVars;
  Var outputVar{NULL_NODE_ID, "output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int sum = 0;
      for (const auto& var : inputVars) {
        if (varNode(var).isFixed()) {
          sum += varNode(var).lowerBound();
        } else {
          sum += _solver->currentValue(varId(var));
        }
      }
      return sum;
    }
    Int sum = 0;
    for (const auto& var : inputVars) {
      EXPECT_TRUE(varNode(var).isFixed());
      sum += varNode(var).lowerBound();
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    for (size_t i = 0; i < 2; ++i) {
      inputVars.emplace_back(Var{NULL_NODE_ID, "input_" + std::to_string(i)});
    }
    if (shouldBeSubsumed()) {
      inputVars.at(0).id = retrieveIntVarNode(1, 1, inputVars.at(0).identifier);
      inputVars.at(1).id = retrieveIntVarNode(1, 1, inputVars.at(1).identifier);
    } else if (shouldBeReplaced()) {
      inputVars.at(0).id = retrieveIntVarNode(0, 0, inputVars.at(0).identifier);
      inputVars.at(1).id =
          retrieveIntVarNode(-2, 2, inputVars.at(1).identifier);
    } else {
      inputVars.at(0).id =
          retrieveIntVarNode(-2, 2, inputVars.at(0).identifier);
      inputVars.at(1).id =
          retrieveIntVarNode(-2, 2, inputVars.at(1).identifier);
    }
    outputVar.id = retrieveIntVarNode(0, 10, outputVar.identifier);

    createInvariantNode(*_invariantGraph, inputVars.at(0).id,
                        inputVars.at(1).id, outputVar.id);
  }
};

TEST_P(IntPlusNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputVarNodeIds.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntPlusNodeTestFixture, application) {
  _solver->open();
  addInputVarsToSolver();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  // a and b
  for (const auto& identifier : inputIdentifiers) {
    const VarNode& inputNode = varNode(identifier);
    if (shouldBeSubsumed()) {
      EXPECT_TRUE(inputNode.isFixed());
    } else if (!shouldBeReplaced()) {
      EXPECT_FALSE(inputNode.isFixed());
    }
    if (!inputNode.isFixed()) {
      EXPECT_TRUE(varId(identifier).isVar());
      EXPECT_THAT(_solver->searchVars(), Contains(size_t(varId(identifier))));
    }
  }
  EXPECT_LE(_solver->searchVars().size(), 2);

  if (!shouldBeSubsumed()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }

  // a, b and outputVarNodeId
  EXPECT_LE(_solver->numVars(), 3);

  // intPow
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(IntPlusNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(IntPlusNodeTestFixture, replace) {
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

TEST_P(IntPlusNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    const VarNode& outputNode = varNode(outputIdentifier);
    EXPECT_TRUE(outputNode.isFixed());
    const Int actual = outputNode.lowerBound();
    const Int expected = computeOutput(true);
    EXPECT_EQ(actual, expected);
    return;
  }

  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputVar).isFixed());
    const VarNodeId addantVarNodeId =
        varNode(varNode(inputVars.front()).isFixed() ? inputVars.back()
                                                     : inputVars.front())
            .varNodeId();
    EXPECT_EQ(varNode(outputVar).varNodeId(), addantVarNodeId);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
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
    IntPlusNodeTest, IntPlusNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
