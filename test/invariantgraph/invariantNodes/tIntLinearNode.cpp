#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;
using ::testing::Contains;

class IntLinearNodeTestFixture : public NodeTestBase<IntLinearNode> {
 public:
  size_t numInputs = 3;
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<Int> coeffs;
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int sum = 0;
      for (size_t i = 0; i < coeffs.size(); ++i) {
        if (coeffs.at(i) == 0) {
          continue;
        }
        if (varNode(inputVarNodeIds.at(i)).isFixed()) {
          sum += varNode(inputVarNodeIds.at(i)).lowerBound() * coeffs.at(i);
        } else {
          sum += _solver->currentValue(varId(inputVarNodeIds.at(i))) *
                 coeffs.at(i);
        }
      }
      return sum;
    }
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      EXPECT_TRUE(varNode(inputVarNodeIds.at(i)).isFixed());
      sum += varNode(inputVarNodeIds.at(i)).lowerBound() * coeffs.at(i);
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds.reserve(numInputs);
    coeffs.reserve(numInputs);
    Int minSum = 0;
    Int maxSum = 0;
    const Int lb = -2;
    const Int ub = 2;
    for (Int i = 0; i < static_cast<Int>(numInputs); ++i) {
      if (shouldBeSubsumed()) {
        const Int val = i % 3 == 0 ? lb : ub;
        inputVarNodeIds.emplace_back(
            retrieveIntVarNode(val, val, "input" + std::to_string(i)));
      } else {
        inputVarNodeIds.emplace_back(
            retrieveIntVarNode(lb, ub, "input" + std::to_string(i)));
      }
      coeffs.emplace_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min(lb * coeffs.back(), ub * coeffs.back());
      maxSum += std::max(lb * coeffs.back(), ub * coeffs.back());
    }

    outputVarNodeId = retrieveIntVarNode(minSum, maxSum, outputIdentifier);

    createInvariantNode(*_invariantGraph, std::vector<Int>(coeffs),
                        std::vector<VarNodeId>(inputVarNodeIds),
                        outputVarNodeId);
  }
};

TEST_P(IntLinearNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(inputVarNodeIds));
  EXPECT_THAT(invNode().outputVarNodeIds(),
              ContainerEq(std::vector<VarNodeId>{outputVarNodeId}));
}

TEST_P(IntLinearNodeTestFixture, application) {
  _solver->open();
  addInputVarsToSolver();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  EXPECT_LE(_solver->searchVars().size(), inputVarNodeIds.size());

  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    EXPECT_THAT(_solver->searchVars(), Contains(varId(inputVarNodeId)));
  }
  EXPECT_LE(_solver->numVars(), inputVarNodeIds.size() + 1);

  // linear invariant
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(IntLinearNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(IntLinearNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(true);
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
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

  const propagation::VarViewId outputId = varId(outputIdentifier);
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

}  // namespace atlantis::testing
