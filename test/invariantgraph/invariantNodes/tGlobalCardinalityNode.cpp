#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class GlobalCardinalityNodeTestFixture
    : public NodeTestBase<GlobalCardinalityNode> {
 public:
  std::vector<VarNodeId> inputs{};
  const std::vector<Int> cover{2, 4};
  std::vector<VarNodeId> outputs{};
  std::vector<std::string> outputIdentifiers;

  std::vector<Int> computeOutputs(propagation::Solver& solver) {
    std::vector<Int> outputs(cover.size(), 0);
    for (const auto& nId : inputs) {
      const Int value = varNode(nId).isFixed()
                            ? varNode(nId).lowerBound()
                            : solver.currentValue(varId(nId));
      for (size_t j = 0; j < cover.size(); ++j) {
        if (value == cover.at(j)) {
          outputs.at(j)++;
        }
      }
    }
    return outputs;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      inputs = {retrieveIntVarNode(2, 2, "input1"),
                retrieveIntVarNode(std::vector<Int>{1, 3, 5}, "input2")};
    } else if (shouldBeReplaced()) {
      inputs = {retrieveIntVarNode(1, 3, "input1"),
                retrieveIntVarNode(1, 3, "input2")};
    } else {
      inputs = {retrieveIntVarNode(1, 5, "input1"),
                retrieveIntVarNode(1, 5, "input2")};
    }

    for (size_t i = 0; i < cover.size(); ++i) {
      outputIdentifiers.emplace_back("output" + std::to_string(i + 1));
      outputs.emplace_back(retrieveIntVarNode(
          0, static_cast<Int>(inputs.size()), outputIdentifiers.back()));
    }

    createInvariantNode(std::vector<VarNodeId>{inputs}, std::vector<Int>{cover},
                        std::vector<VarNodeId>{outputs});
  }
};

TEST_P(GlobalCardinalityNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputs.size());
  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputs);
  EXPECT_THAT(inputs, ContainerEq(invNode().staticInputVarNodeIds()));

  EXPECT_EQ(invNode().outputVarNodeIds().size(), outputs.size());
  EXPECT_EQ(invNode().outputVarNodeIds(), outputs);
  EXPECT_THAT(outputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_P(GlobalCardinalityNodeTestFixture, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);

  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }

  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // input1, input2
  EXPECT_EQ(solver.searchVars().size(), inputs.size());
  // input1, input2, output1, output2
  EXPECT_EQ(solver.numVars(), inputs.size() + outputs.size());
  // gcc
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(GlobalCardinalityNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    for (size_t i = 0; i < cover.size(); ++i) {
      Int expected = 0;
      for (const auto& input : inputs) {
        expected +=
            varNode(input).isFixed() && varNode(input).inDomain(cover.at(i))
                ? 1
                : 0;
      }
      const Int actual = varNode(outputs.at(i)).lowerBound();
      EXPECT_EQ(expected, actual);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    size_t numFixed = 0;
    for (const auto& outputVar : outputs) {
      numFixed += varNode(outputVar).isFixed() ? 1 : 0;
    }
    EXPECT_LT(numFixed, outputs.size());
  }
}

TEST_P(GlobalCardinalityNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_TRUE(invNode().replace(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  }
}

TEST_P(GlobalCardinalityNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  for (size_t i = 0; i < cover.size(); ++i) {
    if (varNode(outputIdentifiers.at(i)).isFixed()) {
      const Int actual = varNode(outputIdentifiers.at(i)).lowerBound();
      Int expected = 0;
      for (const auto& input : inputs) {
        expected +=
            varNode(input).isFixed() && varNode(input).inDomain(cover.at(i))
                ? 1
                : 0;
      }
      EXPECT_EQ(expected, actual);
    }
  }

  std::vector<propagation::VarId> outputIds;
  for (const auto& identifier : outputIdentifiers) {
    outputIds.emplace_back(varNode(identifier).isFixed() ? propagation::NULL_ID
                                                         : varId(identifier));
  }
  bool allNull = true;
  for (const auto& outputId : outputIds) {
    allNull = allNull && outputId == propagation::NULL_ID;
  }

  EXPECT_EQ(allNull, shouldBeSubsumed());
  if (shouldBeSubsumed()) {
    return;
  }

  std::vector<propagation::VarId> inputVars;
  std::vector<Int> inputVals;

  for (const auto& nId : inputs) {
    inputVars.emplace_back(varNode(nId).isFixed() ? propagation::NULL_ID
                                                  : varId(nId));
    inputVals.emplace_back(inputVars.back() == propagation::NULL_ID
                               ? varNode(nId).lowerBound()
                               : solver.lowerBound(inputVars.back()));
  }

  EXPECT_EQ(inputVars.size(), inputVals.size());

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    for (const auto& outputId : outputIds) {
      if (outputId != propagation::NULL_ID) {
        solver.query(outputId);
      }
    }
    solver.endProbe();

    const std::vector<Int> expected = computeOutputs(solver);

    EXPECT_EQ(expected.size(), outputIds.size());
    for (size_t i = 0; i < expected.size(); ++i) {
      const Int actual = outputIds.at(i) == propagation::NULL_ID
                             ? varNode(outputIdentifiers.at(i)).lowerBound()
                             : solver.currentValue(outputIds.at(i));
      EXPECT_EQ(actual, expected.at(i));
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    GlobalCardinalityNodeTest, GlobalCardinalityNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE},
                      ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
