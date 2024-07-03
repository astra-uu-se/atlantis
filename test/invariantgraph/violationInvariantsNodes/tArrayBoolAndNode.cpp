#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class ArrayBoolAndNodeTestFixture : public NodeTestBase<ArrayBoolAndNode> {
 public:
  std::vector<VarNodeId> inputs;
  VarNodeId reified{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};
  Int numInputs = 4;

  bool isViolating(propagation::Solver& solver) {
    for (const auto& input : inputs) {
      if (varNode(input).isFixed()) {
        if (varNode(input).inDomain(bool{false})) {
          return true;
        }
      } else {
        if (solver.currentValue(varId(input)) > 0) {
          return true;
        }
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs.clear();
    inputs.reserve(numInputs);
    for (Int i = 0; i < numInputs; ++i) {
      inputs.emplace_back(retrieveBoolVarNode("input_" + std::to_string(i)));
    }

    if (shouldBeSubsumed()) {
      for (const auto& input : inputs) {
        varNode(input).fixToValue(!shouldFail());
      }
    } else if (shouldBeReplaced()) {
      for (size_t i = 1; i < inputs.size(); ++i) {
        varNode(inputs.at(i)).fixToValue(!shouldFail());
      }
    }

    if (isReified()) {
      reified = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(std::vector<VarNodeId>{inputs}, reified);
    } else if (shouldHold()) {
      createInvariantNode(std::vector<VarNodeId>{inputs}, reified);
    } else {
      createInvariantNode(std::vector<VarNodeId>{inputs}, false);
    }
  }
};

TEST_P(ArrayBoolAndNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numInputs);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputs);
  EXPECT_THAT(inputs, ContainerEq(invNode().staticInputVarNodeIds()));
  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reified);
  }
}

TEST_P(ArrayBoolAndNodeTestFixture, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  EXPECT_EQ(solver.searchVars().size(), numInputs);

  EXPECT_EQ(solver.numVars(), numInputs + 1);

  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(ArrayBoolAndNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reified).isFixed());
      bool expected = false;
      for (const auto& input : inputs) {
        expected = expected || varNode(input).inDomain(bool{false});
      }
      const bool actual = varNode(reified).inDomain(bool{false});
      EXPECT_EQ(expected, actual);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reified).isFixed());
    }
  }
}

TEST_P(ArrayBoolAndNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_TRUE(invNode().replace(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  }
}

TEST_P(ArrayBoolAndNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : inputs) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVars.emplace_back(varId(inputVarNodeId));
    }
  }

  EXPECT_EQ(inputVars.empty(), shouldBeSubsumed());
  if (shouldBeSubsumed()) {
    EXPECT_EQ(isViolating(solver), shouldFail());
    return;
  }

  const propagation::VarId violVarId =
      isReified() ? varId(reifiedIdentifier)
                  : _invariantGraph->totalViolationVarId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(violVarId);
    solver.endProbe();

    const bool actual = solver.currentValue(violVarId) > 0;
    const bool expected = isViolating(solver);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayBoolAndNodeTest, ArrayBoolAndNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
