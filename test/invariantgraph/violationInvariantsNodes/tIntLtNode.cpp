#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLtNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntLtNodeTestFixture : public NodeTestBase<IntLtNode> {
 protected:
  Var aVar{"a", std::vector<Int>{}, false};
  Var bVar{"b", std::vector<Int>{}, false};
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  [[nodiscard]] bool isViolating(const bool isRegistered = false) const {
    if (isRegistered) {
      const Int aVal = varNodeConst(aVar).isFixed()
                           ? varNodeConst(aVar).lowerBound()
                           : _solver->currentValue(varId(aVar));
      const Int bVal = varNodeConst(bVar).isFixed()
                           ? varNodeConst(bVar).lowerBound()
                           : _solver->currentValue(varId(bVar));

      return aVal >= bVal;
    }
    return varNodeConst(aVar).lowerBound() >= varNodeConst(bVar).lowerBound();
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    aVar = retrieveIntVarNode(-5, 5, aVar);
    bVar = retrieveIntVarNode(-5, 5, bVar);
    if (shouldBeSubsumed()) {
      if (shouldHold() || _paramData.data > 0) {
        // varNode(aVarNodeId).removeValuesAbove(0);
        // varNode(bVarNodeId).removeValuesBelow(1);
      } else {
        // varNode(aVarNodeId).removeValuesBelow(0);
        // varNode(bVarNodeId).removeValuesAbove(0);
      }
    }
    if (isReified()) {
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, aVar, bVar, varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, aVar, bVar, shouldHold());
    }
  }
};

TEST_P(IntLtNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    if (isReified()) {
      // TODO: disabled for the MZN challenge. This should be computed by
      // Gecode.
      // EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain(false);
      EXPECT_EQ(expected, actual);
    }
    if (shouldHold()) {
      // TODO: disabled for the MZN challenge. This should be computed by
      // Gecode.
      // EXPECT_FALSE(expected);
    }
    if (shouldFail()) {
      EXPECT_TRUE(expected);
    }
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : std::array<std::string, 2>{aIdentifier, bIdentifier}) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

  EXPECT_NE(violVarId, propagation::NULL_ID);
  EXPECT_EQ(inputVarIds.size(), 2);

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
    IntLtNodeTest, IntLtNodeTestFixture,
    ::testing::Values(ParamData{},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 0},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 1},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
