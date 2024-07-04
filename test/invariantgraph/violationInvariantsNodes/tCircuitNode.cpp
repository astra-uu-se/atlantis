#include <gmock/gmock.h>

#include <iostream>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/circuitNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class CircuitNodeTestFixture : public NodeTestBase<CircuitNode> {
 public:
  Int numInputs = 4;
  std::vector<VarNodeId> inputVarNodeIds;

  InvariantNodeId invNodeId;

  bool isViolating(propagation::Solver& solver) {
    std::vector<Int> values(numInputs, -1);
    for (size_t i = 0; i < inputVarNodeIds.size(); i++) {
      values.at(i) = varNode(inputVarNodeIds.at(i)).isFixed()
                         ? varNode(inputVarNodeIds.at(i)).lowerBound()
                         : solver.currentValue(varId(inputVarNodeIds.at(i)));
    }
    std::vector<bool> visited(numInputs, false);
    Int curNode = 1;
    while (visited.at(curNode - 1) == false) {
      visited.at(curNode - 1) = true;
      curNode = values.at(curNode - 1);
    }
    return std::any_of(visited.begin(), visited.end(),
                       [](bool v) { return !v; });
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    for (Int i = 0; i < numInputs; ++i) {
      std::vector<Int> domain;
      domain.reserve(numInputs - 1);
      for (Int j = 0; j < numInputs; ++j) {
        if (j != i) {
          domain.emplace_back(j + 1);
        }
      }
      inputVarNodeIds.emplace_back(
          retrieveIntVarNode(std::move(domain), "input_" + std::to_string(i)));
    }
    if (shouldBeReplaced()) {
      for (const auto& inputVarNodeId : inputVarNodeIds) {
        _invariantGraph->root().addSearchVarNode(varNode(inputVarNodeId));
      }
    }
    createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds});
  }
};

TEST_P(CircuitNodeTestFixture, makeImplicit) {
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

TEST_P(CircuitNodeTestFixture, propagation) {
  // Currently, we don't allow probes/moves that result in undeterminable
  // dynamic cycles. When the invariant graph is topologically sorted, then an
  // exception should be thrown, and the corresponding probe/move should be
  // ignored/skipped.
  return;
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVarIds.emplace_back(varId(inputVarNodeId));
  }

  const propagation::VarId violVarId = _invariantGraph->totalViolationVarId();
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

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    CircuitNodeTest, CircuitNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::MAKE_IMPLICIT},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
