#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class IntAllEqualNodeTestFixture : public NodeTestBase<IntAllEqualNode> {
 public:
  Int numInputs{4};
  std::vector<VarNodeId> inputs;
  VarNodeId reified{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  bool isViolating(propagation::Solver& solver) {
    std::vector<Int> values;
    values.reserve(inputs.size());
    for (const VarNodeId& input : inputs) {
      values.emplace_back(varNode(input).isFixed()
                              ? varNode(input).lowerBound()
                              : solver.currentValue(varId(input)));
    }

    for (size_t i = 0; i < inputs.size(); ++i) {
      for (size_t j = i + 1; j < inputs.size(); ++j) {
        if (values.at(i) != values.at(j)) {
          return true;
        }
      }
    }

    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    for (Int i = 0; i < numInputs - 1; ++i) {
      if (shouldBeSubsumed()) {
        const Int val = shouldHold() ? 0 : i;
        inputs.emplace_back(
            retrieveIntVarNode(val, val, "input_" + std::to_string(i)));
      } else {
        inputs.emplace_back(
            retrieveIntVarNode(-2, 2, "input_" + std::to_string(i)));
      }
    }
    if (isReified()) {
      reified = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(std::vector<VarNodeId>{inputs}, reified,
                          !shouldBeReplaced());
    } else if (shouldHold()) {
      createInvariantNode(std::vector<VarNodeId>{inputs}, true,
                          !shouldBeReplaced());
    } else {
      createInvariantNode(std::vector<VarNodeId>{inputs}, false,
                          !shouldBeReplaced());
    }
  }
};

TEST_P(IntAllEqualNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

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

TEST_P(IntAllEqualNodeTestFixture, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  for (const auto& input : inputs) {
    EXPECT_THAT(solver.searchVars(), ::testing::Contains(varId(input)));
  }

  EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
  EXPECT_GE(solver.numVars(), invNode().violationVarId(*_invariantGraph));

  EXPECT_EQ(solver.numInvariants(), 1);

  EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
  EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
}

TEST_P(IntAllEqualNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reified).isFixed());
      bool expected = false;
      for (size_t i = 0; i < inputs.size(); ++i) {
        for (size_t j = i + 1; j < inputs.size(); ++j) {
          expected = expected || varNode(inputs.at(i)).lowerBound() !=
                                     varNode(inputs.at(j)).lowerBound();
        }
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

TEST_P(IntAllEqualNodeTestFixture, replace) {
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

TEST_P(IntAllEqualNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : inputs) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
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
    IntAllEqualNodeTest, IntAllEqualNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
