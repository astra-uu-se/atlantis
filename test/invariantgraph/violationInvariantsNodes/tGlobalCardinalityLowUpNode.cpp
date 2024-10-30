#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

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

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      std::vector<Int> counts(cover.size(), 0);
      for (const auto& inputVarNodeId : inputVarNodeIds) {
        const Int val = varNode(inputVarNodeId).isFixed()
                            ? varNode(inputVarNodeId).lowerBound()
                            : _solver->currentValue(varId(inputVarNodeId));
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

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds = {retrieveIntVarNode(5, 10, "x1"),
                       retrieveIntVarNode(2, 7, "x2")};

    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(*_invariantGraph,
                          std::vector<VarNodeId>{inputVarNodeIds},
                          std::vector<Int>{cover}, std::vector<Int>{low},
                          std::vector<Int>{up}, reifiedVarNodeId);
    } else if (shouldHold()) {
      createInvariantNode(*_invariantGraph,
                          std::vector<VarNodeId>{inputVarNodeIds},
                          std::vector<Int>{cover}, std::vector<Int>{low},
                          std::vector<Int>{up}, true);
    } else {
      createInvariantNode(*_invariantGraph,
                          std::vector<VarNodeId>{inputVarNodeIds},
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

  EXPECT_EQ(_solver->searchVars().size(), inputVarNodeIds.size());
  EXPECT_EQ(_solver->numVars(), inputVarNodeIds.size() + 1);

  EXPECT_EQ(_solver->numInvariants(), 1);
  EXPECT_EQ(_solver->lowerBound(invNode().violationVarId()), 0);
  EXPECT_GT(_solver->upperBound(invNode().violationVarId()), 0);
}

TEST_P(GlobalCardinalityLowUpNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

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

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
  }

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedIdentifier)
                  : _invariantGraph->totalViolationVarId();

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
    GlobalCardinalityLowUpNodeTest, GlobalCardinalityLowUpNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
