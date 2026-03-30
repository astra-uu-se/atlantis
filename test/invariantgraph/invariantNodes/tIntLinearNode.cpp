#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;
using ::testing::Contains;

class IntLinearNodeTestFixture : public NodeTestBase<IntLinearNode> {
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
        if (varNode(inputVars.at(i)).isFixed()) {
          sum += varNode(inputVars.at(i)).lowerBound() * coeffs.at(i);
        } else {
          sum += _solver->currentValue(varId(inputVars.at(i))) * coeffs.at(i);
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
      sum += varNode(inputVars.at(i)).lowerBound() * coeffs.at(i);
    }
    return sum;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    inputVars.reserve(numInputs);
    coeffs.reserve(numInputs);
    Int minSum = 0;
    Int maxSum = 0;
    const Int lb = -2;
    const Int ub = 2;
    for (Int i = 0; i < static_cast<Int>(numInputs); ++i) {
      inputVars.emplace_back("input_" + std::to_string(i));
      if (shouldBeSubsumed()) {
        const Int val = i % 3 == 0 ? lb : ub;
        retrieveIntVarNode(val, val, inputVars.back());
      } else {
        retrieveIntVarNode(lb, ub, inputVars.back());
      }
      coeffs.emplace_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min(lb * coeffs.back(), ub * coeffs.back());
      maxSum += std::max(lb * coeffs.back(), ub * coeffs.back());
    }

    retrieveIntVarNode(minSum, maxSum, outputVar);

    createInvariantNode(*_invariantGraph, std::vector<Int>(coeffs),
                        varNodeIds(inputVars), varNodeId(outputVar));
  }
};

TEST_P(IntLinearNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));

  const std::vector<VarNodeId> expectedInputs = varNodeIds(inputVars);
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(expectedInputs));

  const std::vector<VarNodeId> expectedOutputs{varNodeId(outputVar)};

  EXPECT_THAT(invNode().outputVarNodeIds(), ContainerEq(expectedOutputs));
}

TEST_P(IntLinearNodeTestFixture, updateState) {
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

TEST_P(IntLinearNodeTestFixture, propagation) {
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

    const Int actual = _solver->currentValue(outputId);
    const Int expected = computeOutput(true);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    IntLinearNodeTest, IntLinearNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME}));

TEST(IntLinearNodeRegression, MultiInputOffsetUsesOffsetViewForOutput) {
  auto solver = std::make_shared<propagation::Solver>();
  auto graph = std::make_shared<InvariantGraph>();
  graph->open();

  const auto a = graph->retrieveIntVarNode(std::make_shared<SearchDomain>(1, 2),
                                           "a");
  const auto b = graph->retrieveIntVarNode(std::make_shared<SearchDomain>(1, 2),
                                           "b");
  const auto out =
      graph->retrieveIntVarNode(std::make_shared<SearchDomain>(1, 3), "out");

  graph->addInvariantNode(std::make_shared<IntLinearNode>(
      *graph, std::vector<Int>{1, 1}, std::vector<VarNodeId>{a, b}, out, -1));

  graph->close();
  auto mapping = std::make_shared<SolverMapping>(graph->construct(*solver));

  const auto outId = mapping->solverId(out);
  ASSERT_NE(outId, propagation::NULL_ID);

  solver->beginMove();
  solver->setValue(mapping->solverId(a), 1);
  solver->setValue(mapping->solverId(b), 2);
  solver->endMove();

  solver->beginProbe();
  solver->query(outId);
  solver->endProbe();

  EXPECT_EQ(solver->currentValue(outId), 2);
}

}  // namespace atlantis::testing
