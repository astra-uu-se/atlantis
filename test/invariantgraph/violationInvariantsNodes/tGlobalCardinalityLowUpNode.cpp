#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class GlobalCardinalityLowUpNodeTestFixture
    : public NodeTestBase<GlobalCardinalityLowUpNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  const std::vector<Int> cover{2, 6};
  const std::vector<Int> low{0, 1};
  const std::vector<Int> up{1, 2};
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  bool isViolating() {
    std::vector<Int> counts(cover.size(), 0);
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      const Int val = varNode(inputVarNodeId).lowerBound();
      for (size_t i = 0; i < cover.size(); ++i) {
        if (val == cover.at(i)) {
          counts.at(i)++;
          break;
        }
      }
    }
    for (size_t i = 0; i < counts.size(); ++i) {
      if (counts.at(i) < low.at(i) || up.at(i) < counts.at(i)) {
        return true;
      }
    }
    return false;
  }

  bool isViolating(propagation::Solver& solver) {
    std::vector<Int> counts(cover.size(), 0);
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      const Int val = varNode(inputVarNodeId).isFixed()
                          ? varNode(inputVarNodeId).lowerBound()
                          : solver.currentValue(varId(inputVarNodeId));
      for (size_t i = 0; i < cover.size(); ++i) {
        if (val == cover.at(i)) {
          counts.at(i)++;
          break;
        }
      }
    }
    for (size_t i = 0; i < counts.size(); ++i) {
      if (counts.at(i) < low.at(i) || up.at(i) < counts.at(i)) {
        return true;
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds = {retrieveIntVarNode(5, 10, "x1"),
                       retrieveIntVarNode(2, 7, "x2")};

    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds},
                          std::vector<Int>{cover}, std::vector<Int>{low},
                          std::vector<Int>{up}, reifiedVarNodeId);
    } else if (shouldHold()) {
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds},
                          std::vector<Int>{cover}, std::vector<Int>{low},
                          std::vector<Int>{up}, true);
    } else {
      createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds},
                          std::vector<Int>{cover}, std::vector<Int>{low},
                          std::vector<Int>{up}, false);
    }
  }
};

TEST_P(GlobalCardinalityLowUpNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  const size_t numInputs = 2;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numInputs);
  EXPECT_THAT(inputVarNodeIds, ContainerEq(invNode().staticInputVarNodeIds()));

  if (isReified()) {
    EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
    EXPECT_EQ(invNode().outputVarNodeIds().front(), reifiedVarNodeId);
  } else {
    EXPECT_EQ(invNode().outputVarNodeIds().size(), 0);
  }

  if (isReified()) {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reifiedVarNodeId);
  } else {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  }
}

TEST_P(GlobalCardinalityLowUpNodeTestFixture, application) {
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

  EXPECT_EQ(solver.searchVars().size(), inputVarNodeIds.size());
  EXPECT_EQ(solver.numVars(), inputVarNodeIds.size() + 1);

  EXPECT_EQ(solver.numInvariants(), 1);
  EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
  EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
}

TEST_P(GlobalCardinalityLowUpNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);
  _invariantGraph->close(solver);

  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
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
    GlobalCardinalityLowUpNodeTest, GlobalCardinalityLowUpNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
