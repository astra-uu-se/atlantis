#include <gmock/gmock.h>

#include <iostream>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class AllDifferentNodeTestFixture : public NodeTestBase<AllDifferentNode> {
 public:
  Int numInputs = 4;
  std::vector<VarNodeId> inputVarNodeIds;
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  InvariantNodeId invNodeId;

  bool isViolating() {
    for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
      for (size_t j = i + 1; j < inputVarNodeIds.size(); ++j) {
        if (varNode(inputVarNodeIds.at(i)).lowerBound() ==
            varNode(inputVarNodeIds.at(j)).lowerBound()) {
          return true;
        }
      }
    }
    return false;
  }

  bool isViolating(propagation::Solver& solver) {
    for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
      const VarNode& iNode = varNode(inputVarNodeIds.at(i));
      const Int iVal = iNode.isFixed()
                           ? iNode.lowerBound()
                           : solver.currentValue(varId(inputVarNodeIds.at(i)));
      if (!iNode.inDomain(iVal)) {
        return true;
      }
      for (size_t j = i + 1; j < inputVarNodeIds.size(); ++j) {
        const Int jVal =
            varNode(inputVarNodeIds.at(j)).isFixed()
                ? varNode(inputVarNodeIds.at(j)).lowerBound()
                : solver.currentValue(varId(inputVarNodeIds.at(j)));
        if (iVal == jVal) {
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
        inputVarNodeIds.emplace_back(
            retrieveIntVarNode(i, i, "input_" + std::to_string(i)));
      } else {
        inputVarNodeIds.emplace_back(
            retrieveIntVarNode(-2, 2, "input_" + std::to_string(i)));
      }
    }
    if (!shouldBeMadeImplicit()) {
      for (const auto& inputVarNodeId : inputVarNodeIds) {
        _invariantGraph->root().addSearchVarNode(varNode(inputVarNodeId));
      }
    }
    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds},
                          reifiedVarNodeId);
    } else if (shouldHold()) {
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds}, true);
    } else {
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds}, false);
    }
  }
};

TEST_P(AllDifferentNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(inputVarNodeIds, ContainerEq(invNode().staticInputVarNodeIds()));

  if (isReified()) {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reifiedVarNodeId);
  } else {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  }
}

TEST_P(AllDifferentNodeTestFixture, application) {
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

  for (const auto& inputVarNodeId : inputVarNodeIds) {
    EXPECT_THAT(solver.searchVars(),
                ::testing::Contains(varId(inputVarNodeId)));
  }

  EXPECT_GE(solver.numVars(), invNode().violationVarId(*_invariantGraph));

  // alldifferent
  EXPECT_EQ(solver.numInvariants(), 1);

  EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
  EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
}

TEST_P(AllDifferentNodeTestFixture, makeImplicit) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeMadeImplicit()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeMadeImplicit(*_invariantGraph));
    EXPECT_TRUE(invNode().makeImplicit(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  }
}

TEST_P(AllDifferentNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);

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

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
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
    AllDifferentNodeTest, AllDifferentNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::MAKE_IMPLICIT}));

}  // namespace atlantis::testing
