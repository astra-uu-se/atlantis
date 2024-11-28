#include <gmock/gmock.h>

#include <iostream>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/circuitNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class CircuitNodeTestFixture : public NodeTestBase<CircuitNode> {
 public:
  Int numInputs = 4;
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers;

  bool isViolating(bool) {
    std::vector<Int> values(numInputs, -1);
    for (size_t i = 0; i < inputIdentifiers.size(); i++) {
      values.at(i) = varNode(inputIdentifiers.at(i)).isFixed()
                         ? varNode(inputIdentifiers.at(i)).lowerBound()
                         : _solver->currentValue(varId(inputIdentifiers.at(i)));
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
      inputIdentifiers.emplace_back("input_" + std::to_string(i));
      inputVarNodeIds.emplace_back(
          retrieveIntVarNode(std::move(domain), inputIdentifiers.back()));
    }
    if (shouldBeReplaced()) {
      for (const auto& inputVarNodeId : inputVarNodeIds) {
        _invariantGraph->root().addSearchVarNode(inputVarNodeId);
      }
    }
    createInvariantNode(*_invariantGraph,
                        std::vector<VarNodeId>{inputVarNodeIds}, 1);
  }
};

TEST_P(CircuitNodeTestFixture, makeImplicit) {
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

TEST_P(CircuitNodeTestFixture, propagation) {
  // Currently, we don't allow probes/moves that result in undeterminable
  // dynamic cycles. When the invariant graph is topologically sorted, then an
  // exception should be thrown, and the corresponding probe/move should be
  // ignored/skipped.
  if (shouldBeMadeImplicit()) {
    return;
  }
  return;
  propagation::Solver solver;
  _invariantGraph->construct();
  for (Int i = 0; i < numInputs; i++) {
    const Int val = 1 + ((i + 1) % numInputs);
    _solver->setValue(varId(inputIdentifiers.at(i)), val);
  }
  _invariantGraph->close();

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVarIds.emplace_back(varId(inputVarNodeId));
  }

  const propagation::VarViewId violVarId =
      _invariantGraph->totalViolationVarId();
  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    try {
      _solver->beginProbe();
      _solver->query(violVarId);
      _solver->endProbe();
    } catch (TopologicalOrderError& e) {
      EXPECT_TRUE(isViolating(true));
      continue;
    }

    expectVarVals(inputVarIds, inputVals);

    const bool actual = _solver->currentValue(violVarId) > 0;
    const bool expected = isViolating(true);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    CircuitNodeTest, CircuitNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::MAKE_IMPLICIT},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
