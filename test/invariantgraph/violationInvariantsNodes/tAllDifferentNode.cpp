#include <gmock/gmock.h>

#include <iostream>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class AllDifferentNodeTestFixture : public NodeTestBase<AllDifferentNode> {
 public:
  Int numInputs = 4;
  std::vector<Var> inputVars;
  Var reifiedVar{NULL_NODE_ID, "reified"};

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      for (size_t i = 0; i < inputVars.size(); ++i) {
        const VarNode& iNode = varNode(inputVars.at(i));
        const Int iVal = iNode.isFixed()
                             ? iNode.lowerBound()
                             : _solver->currentValue(varId(inputVars.at(i)));
        if (!iNode.inDomain(iVal)) {
          return true;
        }
        for (size_t j = i + 1; j < inputVars.size(); ++j) {
          const Int jVal = varNode(inputVars.at(j)).isFixed()
                               ? varNode(inputVars.at(j)).lowerBound()
                               : _solver->currentValue(varId(inputVars.at(j)));
          if (iVal == jVal) {
            return true;
          }
        }
      }
      return false;
    }
    for (size_t i = 0; i < inputVars.size(); ++i) {
      for (size_t j = i + 1; j < inputVars.size(); ++j) {
        if (varNode(inputVars.at(i)).lowerBound() ==
            varNode(inputVars.at(j)).lowerBound()) {
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
        inputVars.emplace_back(makeIntVar(i, i, "input_" + std::to_string(i)));
      } else {
        inputVars.emplace_back(makeIntVar(-2, 2, "input_" + std::to_string(i)));
      }
    }
    if (!shouldBeMadeImplicit()) {
      for (const auto& var : inputVars) {
        _invariantGraph->root().addSearchVarNode(var.id);
      }
    }
    if (isReified()) {
      reifiedVar.id = retrieveBoolVarNode(reifiedVar.identifier);
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          reifiedVar.id);
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          shouldHold());
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
  EXPECT_NE(invNode().violationVarId(), propagation::NULL_ID);
  invNode().registerNode();
  _solver->close();

  for (const auto& inputVarNodeId : inputVarNodeIds) {
    EXPECT_TRUE(varId(inputVarNodeId).isVar());
    EXPECT_THAT(_solver->searchVars(),
                ::testing::Contains(size_t(varId(inputVarNodeId))));
  }

  EXPECT_GE(_solver->numVars(), size_t(invNode().violationVarId()));

  // alldifferent
  EXPECT_EQ(_solver->numInvariants(), 1);

  EXPECT_EQ(_solver->lowerBound(invNode().violationVarId()), 0);
  EXPECT_GT(_solver->upperBound(invNode().violationVarId()), 0);
}

TEST_P(AllDifferentNodeTestFixture, makeImplicit) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeMadeImplicit()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeMadeImplicit());
    EXPECT_TRUE(invNode().makeImplicit());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  }
}

TEST_P(AllDifferentNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    const bool expected = isViolating(true);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain({false});
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

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _invariantGraph->totalViolationVarId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(violVarId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const bool actual = _solver->currentValue(violVarId) > 0;
    const bool expected = isViolating(true);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    AllDifferentNodeTest, AllDifferentNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::MAKE_IMPLICIT}));

}  // namespace atlantis::testing
