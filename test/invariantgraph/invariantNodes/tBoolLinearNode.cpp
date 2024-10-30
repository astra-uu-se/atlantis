#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;

class BoolLinearNodeTestFixture : public NodeTestBase<BoolLinearNode> {
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
        if (varNode(inputVarNodeIds.at(i)).isFixed() ||
            varId(inputVarNodeIds.at(i)) == propagation::NULL_ID) {
          sum += varNode(inputVarNodeIds.at(i)).inDomain(bool{true})
                     ? coeffs.at(i)
                     : 0;
        } else {
          sum += _solver->currentValue(varId(inputVarNodeIds.at(i))) == 0
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
      EXPECT_TRUE(varNode(inputVarNodeIds.at(i)).isFixed());
      sum += varNode(inputVarNodeIds.at(i)).inDomain(bool{true}) ? coeffs.at(i)
                                                                 : 0;
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds.reserve(numInputs);
    coeffs.reserve(numInputs);
    Int minSum = 0;
    Int maxSum = 0;
    for (size_t i = 0; i < numInputs; ++i) {
      inputVarNodeIds.push_back(
          retrieveBoolVarNode("input" + std::to_string(i)));
      if (shouldBeSubsumed()) {
        _invariantGraph->varNode(inputVarNodeIds.back())
            .fixToValue(bool{i % 2 == 0});
      }
      coeffs.push_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min<Int>(coeffs.back(), 0);
      maxSum += std::max<Int>(coeffs.back(), 0);
    }

    outputVarNodeId = retrieveIntVarNode(minSum, maxSum, outputIdentifier);

    createInvariantNode(*_invariantGraph, std::vector<Int>(coeffs),
                        std::vector<VarNodeId>(inputVarNodeIds),
                        outputVarNodeId);
  }
};

TEST_P(BoolLinearNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(inputVarNodeIds));
  EXPECT_THAT(invNode().outputVarNodeIds(),
              std::vector<VarNodeId>{outputVarNodeId});
}

TEST_P(BoolLinearNodeTestFixture, application) {
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

  // inputVarNodeIds and outputVarNodeId
  EXPECT_EQ(_solver->searchVars().size(), inputVarNodeIds.size());

  // inputVarNodeIds and outputVarNodeId
  EXPECT_EQ(_solver->numVars(), inputVarNodeIds.size() + 1);

  // linear invariant
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(BoolLinearNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(BoolLinearNodeTestFixture, propagation) {
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

  const propagation::VarViewId outputVarId = varId(outputIdentifier);
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

INSTANTIATE_TEST_CASE_P(
    BoolLinearNodeTest, BoolLinearNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
