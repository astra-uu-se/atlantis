#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntModNodeTestFixture : public NodeTestBase<IntModNode> {
 public:
  std::string numeratorVar{"numerator"};
  std::string denominatorVar{"denominator"};
  std::string outputVar{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int numerator = varNode(numeratorVar).isFixed()
                                ? varNode(numeratorVar).lowerBound()
                                : _solver->currentValue(varId(numeratorVar));
      const Int denominator =
          varNode(denominatorVar).isFixed()
              ? varNode(denominatorVar).lowerBound()
              : _solver->currentValue(varId(denominatorVar));
      return denominator != 0 ? numerator % denominator : 0;
    }
    const Int numerator = varNode(numeratorVar).lowerBound();
    const Int denominator = varNode(denominatorVar).lowerBound();
    return denominator != 0 ? numerator % denominator : 0;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    retrieveIntVarNode(0, 6, numeratorVar);
    retrieveIntVarNode(1, 10, denominatorVar);
    retrieveIntVarNode(0, 10, outputVar);

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
  for (const auto& var :
       std::array<std::string, 2>{numeratorVar, denominatorVar}) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
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
