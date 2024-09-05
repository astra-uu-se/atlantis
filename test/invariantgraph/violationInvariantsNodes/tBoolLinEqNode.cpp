#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLinEqNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;
using ::testing::Contains;

class BoolLinEqNodeTestFixture : public NodeTestBase<BoolLinEqNode> {
 public:
  size_t numInputs = 3;
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<Int> coeffs;
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"output"};
  Int bound = -1;

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      Int sum = 0;
      for (size_t i = 0; i < coeffs.size(); ++i) {
        if (coeffs.at(i) == 0) {
          continue;
        }
        if (varNode(inputVarNodeIds.at(i)).isFixed()) {
          sum += varNode(inputVarNodeIds.at(i)).inDomain(bool{true})
                     ? coeffs.at(i)
                     : 0;
        } else {
          sum += _solver->currentValue(varId(inputVarNodeIds.at(i))) == 0
                     ? coeffs.at(i)
                     : 0;
        }
      }
      return sum != bound;
    }
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      EXPECT_TRUE(varNode(inputVarNodeIds.at(i)).isFixed());
      sum += varNode(inputVarNodeIds.at(i)).inDomain(bool{true}) ? coeffs.at(i)
                                                                 : 0;
    }
    return sum != bound;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds.reserve(numInputs);
    coeffs.reserve(numInputs);
    for (Int i = 0; i < static_cast<Int>(numInputs); ++i) {
      inputVarNodeIds.emplace_back(
          retrieveBoolVarNode("input" + std::to_string(i)));
      if (shouldBeSubsumed()) {
        varNode(inputVarNodeIds.back()).fixToValue(bool{i % 3 == 0});
      }
      coeffs.emplace_back((i + 1) * (i % 2 == 0 ? -1 : 1));
    }

    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(*_invariantGraph, std::vector<Int>(coeffs),
                          std::vector<VarNodeId>(inputVarNodeIds), bound,
                          reifiedVarNodeId);
    } else {
      createInvariantNode(*_invariantGraph, std::vector<Int>(coeffs),
                          std::vector<VarNodeId>(inputVarNodeIds), bound,
                          shouldHold());
    }
  }
};

TEST_P(BoolLinEqNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(inputVarNodeIds));
  if (isReified()) {
    EXPECT_THAT(invNode().outputVarNodeIds(),
                ContainerEq(std::vector<VarNodeId>{reifiedVarNodeId}));
  }
}

TEST_P(BoolLinEqNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    const Int expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVarNodeId).isFixed());
      const Int actual = varNode(reifiedVarNodeId).lowerBound();
      EXPECT_EQ(expected, actual);
    } else if (shouldHold()) {
      EXPECT_FALSE(expected);
    } else {
      EXPECT_TRUE(expected);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reifiedVarNodeId).isFixed());
    }
  }
}

TEST_P(BoolLinEqNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

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

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
  }

  EXPECT_FALSE(inputVarIds.empty());

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedIdentifier)
                  : _invariantGraph->totalViolationVarId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals)) {
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
    BoolLinEqNodeTest, BoolLinEqNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
