#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class BoolAllEqualNodeTestFixture : public NodeTestBase<BoolAllEqualNode> {
 public:
  Int numInputs{4};
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers;
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  bool isViolating() {
    bool allSameVarNodeId = true;
    for (size_t i = 0; i < inputIdentifiers.size(); ++i) {
      for (size_t j = i + 1; j < inputIdentifiers.size(); ++j) {
        if (varNode(inputIdentifiers.at(i)).varNodeId() !=
            varNode(inputIdentifiers.at(j)).varNodeId()) {
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
    for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
      for (size_t j = i + 1; j < inputVarNodeIds.size(); ++j) {
        if (varNode(inputVarNodeIds.at(i)).inDomain(bool{true}) !=
            varNode(inputVarNodeIds.at(j)).inDomain(bool{true})) {
          return true;
        }
      }
    }
    return false;
  }

  bool isViolating(propagation::Solver& solver) {
    bool allSameVarNodeId = true;
    for (size_t i = 0; i < inputIdentifiers.size(); ++i) {
      for (size_t j = i + 1; j < inputIdentifiers.size(); ++j) {
        if (varNode(inputIdentifiers.at(i)).varNodeId() !=
            varNode(inputIdentifiers.at(j)).varNodeId()) {
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
    for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
      const bool iVal =
          varNode(inputVarNodeIds.at(i)).isFixed()
              ? varNode(inputVarNodeIds.at(i)).inDomain(bool{true})
              : solver.currentValue(varId(inputVarNodeIds.at(i))) == 0;
      for (size_t j = i + 1; j < inputVarNodeIds.size(); ++j) {
        const bool jVal =
            varNode(inputVarNodeIds.at(j)).isFixed()
                ? varNode(inputVarNodeIds.at(j)).inDomain(bool{true})
                : solver.currentValue(varId(inputVarNodeIds.at(j))) == 0;
        if (iVal != jVal) {
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
      inputIdentifiers.emplace_back("input_" + std::to_string(i));
      inputVarNodeIds.emplace_back(
          retrieveBoolVarNode(inputIdentifiers.back()));
      if (shouldBeSubsumed()) {
        const bool val = shouldHold() || i == 0;
        varNode(inputIdentifiers.back()).fixToValue(val);
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
                          reifiedVarNodeId, !shouldBeReplaced());
    } else if (shouldHold()) {
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds}, true,
                          !shouldBeReplaced());
    } else {
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds}, false,
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

  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(BoolAllEqualNodeTestFixture, updateState) {
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

TEST_P(BoolAllEqualNodeTestFixture, replace) {
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

TEST_P(BoolAllEqualNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);
  _invariantGraph->close(solver);

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
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
  for (const auto& inputIdentifier : inputIdentifiers) {
    if (!varNode(inputIdentifier).isFixed()) {
      const propagation::VarId inputVarId = varId(inputIdentifier);
      EXPECT_NE(inputVarId, propagation::NULL_ID);
      const bool inVec = std::any_of(
          inputVarIds.begin(), inputVarIds.end(),
          [&](const propagation::VarId& varId) { return varId == inputVarId; });
      if (!inVec) {
        inputVarIds.emplace_back(inputVarId);
      }
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
