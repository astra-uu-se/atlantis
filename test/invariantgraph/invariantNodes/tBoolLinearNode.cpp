#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;

class BoolLinearNodeTestFixture : public NodeTestBase<BoolLinearNode> {
 public:
  size_t numInputs = 3;
  std::vector<std::string> inputVars;
  std::vector<Int> coeffs;
  std::string outputVar{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int sum = 0;
      for (size_t i = 0; i < coeffs.size(); ++i) {
        if (coeffs.at(i) == 0) {
          continue;
        }
        if (varNode(inputVars.at(i)).isFixed() ||
            varId(inputVars.at(i)) == propagation::NULL_ID) {
          sum +=
              varNode(inputVars.at(i)).inDomain(bool{true}) ? coeffs.at(i) : 0;
        } else {
          sum += _solver->currentValue(varId(inputVars.at(i))) == 0
                     ? coeffs.at(i)
                     : 0;
        }
      }
      return sum;
    }
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      EXPECT_TRUE(varNode(inputVars.at(i)).isFixed());
      sum += varNode(inputVars.at(i)).inDomain(bool{true}) ? coeffs.at(i) : 0;
    }
    return sum;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    inputVars.reserve(numInputs);
    coeffs.reserve(numInputs);
    Int minSum = 0;
    Int maxSum = 0;
    for (size_t i = 0; i < numInputs; ++i) {
      inputVars.emplace_back("input_" + std::to_string(i));
      retrieveBoolVarNode(inputVars.back());
      if (shouldBeSubsumed()) {
        varNode(inputVars.back()).fixToValue(bool{i % 2 == 0});
      }
      coeffs.push_back((static_cast<Int>(i) + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min<Int>(coeffs.back(), 0);
      maxSum += std::max<Int>(coeffs.back(), 0);
    }

    retrieveIntVarNode(minSum, maxSum, outputVar);

    createInvariantNode(*_invariantGraph, std::vector<Int>(coeffs),
                        varNodeIds(inputVars), varNodeId(outputVar));
  }
};

TEST_P(BoolLinearNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(BoolLinearNodeTestFixture, propagation) {
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
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  EXPECT_FALSE(inputVarIds.empty());

  const propagation::VarViewId outputVarId = varId(outputVar);
  EXPECT_NE(outputVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputVarId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int actual = _solver->currentValue(outputVarId);
    const Int expected = computeOutput(true);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_SUITE_P(
    BoolLinearNodeTest, BoolLinearNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
