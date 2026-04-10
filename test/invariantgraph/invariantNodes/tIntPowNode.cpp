#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntPowNodeTestFixture : public NodeTestBase<IntPowNode> {
 public:
  std::string baseVar{"base"};
  std::string exponentVar{"exponent"};
  std::string outputVar{"output"};

  [[nodiscard]] static Int int_exp(Int baseVal, Int exponentVal) {
    if (exponentVal == 0) {
      return 1;
    }
    if (exponentVal == 1) {
      return baseVal;
    }
    if (exponentVal < 0) {
      EXPECT_NE(baseVal, 0);
      if (baseVal == 1) {
        return 1;
      }
      if (baseVal == -1) {
        return exponentVal % 2 == 0 ? 1 : -1;
      }
      return 0;
    }
    Int result = 1;
    for (int i = 0; i < exponentVal; i++) {
      result *= baseVal;
    }
    return result;
  }

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int baseVal = varNode(baseVar).isFixed()
                              ? varNode(baseVar).lowerBound()
                              : _solver->currentValue(varId(baseVar));
      const Int exponentVal = varNode(exponentVar).isFixed()
                                  ? varNode(exponentVar).lowerBound()
                                  : _solver->currentValue(varId(exponentVar));
      return int_exp(baseVal, exponentVal);
    }
    const Int baseVal = varNode(baseVar).lowerBound();
    const Int exponentVal = varNode(exponentVar).lowerBound();
    return int_exp(baseVal, exponentVal);
  }

  void SetUp() {
    NodeTestBase::SetUp();
    retrieveIntVarNode(0, 10, baseVar);
    retrieveIntVarNode(0, 10, exponentVar);
    retrieveIntVarNode(0, 10, outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(baseVar),
                        varNodeId(exponentVar), varNodeId(outputVar));
  }
};

TEST_P(IntPowNodeTestFixture, propagation) {
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
  for (const auto& var : std::array<std::string, 2>{baseVar, exponentVar}) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    if (inputVals.at(0) != 0 || inputVals.at(1) > 0) {
      const Int actual = _solver->currentValue(outputId);
      const Int expected = computeOutput(true);
      EXPECT_EQ(actual, expected);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(IntPowNodeTest, IntPowNodeTestFixture,
                        ::testing::Values(ParamData{
                            InvariantNodeAction::NONE}));

}  // namespace atlantis::testing
