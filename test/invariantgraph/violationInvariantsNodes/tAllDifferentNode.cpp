#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class AllDifferentNodeTestFixture : public NodeTestBase<AllDifferentNode> {
 protected:
  Int numInputs{4};
  std::vector<Var> inputVars;
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  [[nodiscard]] bool isViolating(const bool isRegistered = false) const {
    if (isRegistered) {
      for (size_t i = 0; i < inputVars.size(); ++i) {
        const VarNode& iNode = varNodeConst(inputVars.at(i));
        const Int iVal = iNode.isFixed()
                             ? iNode.lowerBound()
                             : _solver->currentValue(varId(inputVars.at(i)));
        if (!iNode.inDomain(iVal)) {
          return true;
        }
        for (size_t j = i + 1; j < inputVars.size(); ++j) {
          const Int jVal = varNodeConst(inputVars.at(j)).isFixed()
                               ? varNodeConst(inputVars.at(j)).lowerBound()
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
        if (varNodeConst(inputVars.at(i)).lowerBound() ==
            varNodeConst(inputVars.at(j)).lowerBound()) {
          return true;
        }
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    for (Int i = 0; i < numInputs; ++i) {
      inputVars.emplace_back("input_" + std::to_string(i),
                             shouldBeSubsumed() ? i : -2,
                             shouldBeSubsumed() ? i : 2, true);
      retrieveIntVarNode(inputVars.back());
    }
    if (!shouldBeMadeImplicit()) {
      for (const auto& var : inputVars) {
        _invariantGraph->root().addSearchVarNode(varNodeId(var));
      }
    }
    if (isReified()) {
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          shouldHold());
    }
  }
};

TEST_P(AllDifferentNodeTestFixture, construction) {
  addInputVarsToSolver();
  expectInputTo(invNode());
  expectOutputOf(invNode());

  std::vector<VarNodeId> expectedInputs = varNodeIds(inputVars);

  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  if (isReified()) {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), varNodeId(reifiedVar));
  } else {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  }
}

TEST_P(AllDifferentNodeTestFixture, application) {
  _solver->open();
  _solverMapping = std::make_shared<SolverMapping>();
  addInputVarsToSolver();
  EXPECT_EQ(_solverMapping->violationId(_invNodeId), propagation::NULL_ID);
  if (invNode().isReified()) {
    EXPECT_EQ(invNode().outputVarNodeIds().size(), size_t{1});
    EXPECT_EQ(_solverMapping->solverId(invNode().outputVarNodeIds().front()),
              propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_solver, *_solverMapping);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  const propagation::VarViewId violationId =
      invNode().isReified()
          ? _solverMapping->solverId(invNode().outputVarNodeIds().front())
          : _solverMapping->violationId(_invNodeId);
  EXPECT_NE(violationId, propagation::NULL_ID);
  invNode().registerNode(*_solver, *_solverMapping);
  _solver->close();

  for (const auto& input : inputVars) {
    EXPECT_TRUE(varId(input).isVar());
    EXPECT_THAT(_solver->searchVars(),
                ::testing::Contains(size_t(varId(input))));
  }

  EXPECT_GE(_solver->numVars(), size_t(violationId));

  // alldifferent
  EXPECT_EQ(_solver->numInvariants(), 1);

  EXPECT_EQ(_solver->lowerBound(violationId), 0);
  EXPECT_GT(_solver->upperBound(violationId), 0);
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
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    const bool expected = isViolating(true);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain(false);
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
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

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

INSTANTIATE_TEST_SUITE_P(
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
