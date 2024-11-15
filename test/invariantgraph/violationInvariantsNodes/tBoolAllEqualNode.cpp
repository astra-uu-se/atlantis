#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class BoolAllEqualNodeTestFixture : public NodeTestBase<BoolAllEqualNode> {
 public:
  Int numInputs{4};
  std::vector<Var> inputVars;

  Var reifiedVar{NULL_NODE_ID, "reified"};

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      bool allSameVarNodeId = true;
      for (size_t i = 0; i < inputVars.size(); ++i) {
        for (size_t j = i + 1; j < inputVars.size(); ++j) {
          if (varNode(inputVars.at(i)).varNodeId() !=
              varNode(inputVars.at(j)).varNodeId()) {
            allSameVarNodeId = false;
            break;
          }
        }
        if (!allSameVarNodeId) {
          break;
        }
      }
      if (allSameVarNodeId) {
        return false;
      }
      for (size_t i = 0; i < inputVars.size(); ++i) {
        const bool iVal =
            varNode(inputVars.at(i)).isFixed()
                ? varNode(inputVars.at(i)).inDomain(bool{true})
                : _solver->currentValue(varId(inputVars.at(i))) == 0;
        for (size_t j = i + 1; j < inputVars.size(); ++j) {
          const bool jVal =
              varNode(inputVars.at(j)).isFixed()
                  ? varNode(inputVars.at(j)).inDomain(bool{true})
                  : _solver->currentValue(varId(inputVars.at(j))) == 0;
          if (iVal != jVal) {
            return true;
          }
        }
      }
      return false;
    }
    bool allSameVarNodeId = true;
    for (size_t i = 0; i < inputVars.size(); ++i) {
      for (size_t j = i + 1; j < inputVars.size(); ++j) {
        if (varNode(inputVars.at(i)).varNodeId() !=
            varNode(inputVars.at(j)).varNodeId()) {
          allSameVarNodeId = false;
          break;
        }
      }
      if (!allSameVarNodeId) {
        break;
      }
    }
    if (allSameVarNodeId) {
      return false;
    }
    for (size_t i = 0; i < inputVars.size(); ++i) {
      for (size_t j = i + 1; j < inputVars.size(); ++j) {
        if (varNode(inputVars.at(i)).inDomain(bool{true}) !=
            varNode(inputVars.at(j)).inDomain(bool{true})) {
          return true;
        }
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    numInputs = !shouldBeReplaced() || shouldHold() ? 4 : 2;

    for (Int i = 0; i < numInputs; ++i) {
      inputVars.emplace_back(
          makeBoolVar("input_" + std::to_string(inputVars.size())));
      if (shouldBeSubsumed()) {
        const bool val = shouldHold() || i == 0;
        varNode(inputVars.back()).fixToValue(val);
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
                          reifiedVar.id, !shouldBeReplaced());
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars), shouldHold(),
                          !shouldBeReplaced());
    }
  }
};

TEST_P(BoolAllEqualNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

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

TEST_P(BoolAllEqualNodeTestFixture, application) {
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

  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(BoolAllEqualNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool expected = isViolating();
      const bool actual = varNode(reifiedVar).inDomain(bool{false});
      EXPECT_EQ(expected, actual);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reifiedVar).isFixed());
    }
  }
}

TEST_P(BoolAllEqualNodeTestFixture, replace) {
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

TEST_P(BoolAllEqualNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
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
      const propagation::VarViewId inputVarId = varId(var);
      EXPECT_NE(inputVarId, propagation::NULL_ID);
      const bool inVec =
          std::ranges::any_of(inputVarIds.begin(), inputVarIds.end(),
                              [&](const propagation::VarViewId& varId) {
                                return varId == inputVarId;
                              });
      if (!inVec) {
        inputVarIds.emplace_back(inputVarId);
      }
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
    BoolAllEqualNodeTest, BoolAllEqualNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
