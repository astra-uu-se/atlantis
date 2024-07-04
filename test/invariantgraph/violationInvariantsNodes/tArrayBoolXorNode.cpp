#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class ArrayBoolXorNodeTestFixture : public NodeTestBase<ArrayBoolXorNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};
  Int numInputs = 4;

  bool isViolating() {
    bool trueFound = false;
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      if (varNode(inputVarNodeId).inDomain(bool{true})) {
        if (trueFound) {
          return true;
        }
        trueFound = true;
      }
    }
    return !trueFound;
  }

  bool isViolating(propagation::Solver& solver) {
    bool trueFound = false;
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      if (varNode(inputVarNodeId).isFixed()) {
        if (varNode(inputVarNodeId).inDomain(bool{true})) {
          if (trueFound) {
            return true;
          }
          trueFound = true;
        }
      } else {
        if (solver.currentValue(varId(inputVarNodeId)) == 0) {
          if (trueFound) {
            return true;
          }
          trueFound = true;
        }
      }
    }
    return !trueFound;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds.clear();
    inputVarNodeIds.reserve(numInputs);
    for (Int i = 0; i < numInputs; ++i) {
      inputVarNodeIds.emplace_back(
          retrieveBoolVarNode("input_" + std::to_string(i)));
    }

    if (shouldBeSubsumed()) {
      for (const auto& inputVarNodeId : inputVarNodeIds) {
        varNode(inputVarNodeId).fixToValue(!shouldFail());
      }
    } else if (shouldBeReplaced()) {
      for (size_t i = 1; i < inputVarNodeIds.size(); ++i) {
        varNode(inputVarNodeIds.at(i)).fixToValue(!shouldFail());
      }
    }

    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds},
                          reifiedVarNodeId);
    } else if (shouldHold()) {
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds},
                          reifiedVarNodeId);
    } else {
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds}, false);
    }
  }
};

TEST_P(ArrayBoolXorNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numInputs);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputVarNodeIds);
  EXPECT_THAT(inputVarNodeIds, ContainerEq(invNode().staticInputVarNodeIds()));
  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reifiedVarNodeId);
  }
}

TEST_P(ArrayBoolXorNodeTestFixture, application) {
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

TEST_P(ArrayBoolXorNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVarNodeId).isFixed());
      const bool expected = isViolating();
      const bool actual = varNode(reifiedVarNodeId).inDomain(bool{false});
      EXPECT_EQ(expected, actual);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reifiedVarNodeId).isFixed());
    }
  }
}

TEST_P(ArrayBoolXorNodeTestFixture, replace) {
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

TEST_P(ArrayBoolXorNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& inputNodeId : inputVarNodeIds) {
    if (!varNode(inputNodeId).isFixed()) {
      EXPECT_NE(varId(inputNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputNodeId));
    }
  }

  EXPECT_EQ(inputVarIds.empty(), shouldBeSubsumed());
  if (shouldBeSubsumed()) {
    const bool expected = isViolating(solver);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedIdentifier).isFixed());
      const bool actual = varNode(reifiedIdentifier).inDomain({false});
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

  const propagation::VarId violVarId =
      isReified() ? varId(reifiedIdentifier)
                  : _invariantGraph->totalViolationVarId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(solver, inputVarIds);

  while (increaseNextVal(solver, inputVarIds, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVarIds, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(violVarId);
    solver.endProbe();

    expectVarVals(solver, inputVarIds, inputVals);

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
    ArrayBoolXorNodeTest, ArrayBoolXorNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
