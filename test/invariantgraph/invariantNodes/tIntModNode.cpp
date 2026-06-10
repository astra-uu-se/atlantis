#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntModNodeTestFixture : public NodeTestBase<IntModNode> {
 protected:
  Var numeratorVar{"numerator", std::vector<Int>{}, true};
  Var denominatorVar{"denominator", std::vector<Int>{}, true};
  Var outputVar{"output", std::vector<Int>{}, true};

  [[nodiscard]] Int computeOutput(const bool isRegistered = false) const {
    if (isRegistered) {
      const Int numerator = varNodeConst(numeratorVar).isFixed()
                                ? varNodeConst(numeratorVar).lowerBound()
                                : _solver->currentValue(varId(numeratorVar));
      const Int denominator =
          varNodeConst(denominatorVar).isFixed()
              ? varNodeConst(denominatorVar).lowerBound()
              : _solver->currentValue(varId(denominatorVar));
      return denominator != 0 ? numerator % denominator : 0;
    }
    const Int numerator = varNodeConst(numeratorVar).lowerBound();
    const Int denominator = varNodeConst(denominatorVar).lowerBound();
    return denominator != 0 ? numerator % denominator : 0;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    numeratorVar.domain = std::pair<Int, Int>{0, 6};
    denominatorVar.domain = std::pair<Int, Int>{1, 10};
    outputVar.domain = std::pair<Int, Int>{0, 10};
    retrieveIntVarNode(numeratorVar);
    retrieveIntVarNode(denominatorVar);
    retrieveIntVarNode(outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(numeratorVar),
                        varNodeId(denominatorVar), varNodeId(outputVar));
  }
};

TEST_P(IntModNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(true);
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& vId : std::array<VarNodeId, 2>{varNodeId(numeratorVar),
                                                  varNodeId(denominatorVar)}) {
    if (!varNode(vId).isFixed()) {
      EXPECT_NE(varId(vId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(vId));
    }
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarViewId outputId =
      varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputVarIds.size(), 2);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    if (inputVals.at(1) != 0) {
      const Int actual = _solver->currentValue(outputId);
      const Int expected = computeOutput(true);
      EXPECT_EQ(actual, expected);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(IntModNodeTest, IntModNodeTestFixture,
                         ::testing::Values(ParamData{
                             InvariantNodeAction::NONE}));

}  // namespace atlantis::testing
