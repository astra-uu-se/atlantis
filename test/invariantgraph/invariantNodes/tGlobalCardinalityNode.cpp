#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static std::vector<Int> computeOutputs(const std::vector<Int>& values,
                                       const std::vector<Int>& cover) {
  std::vector<Int> outputs(cover.size(), 0);
  for (Int value : values) {
    for (size_t j = 0; j < cover.size(); ++j) {
      if (value == cover.at(j)) {
        outputs.at(j)++;
      }
    }
  }
  return outputs;
}

class GlobalCardinalityNodeTest : public NodeTestBase<GlobalCardinalityNode> {
 public:
  VarNodeId input1{NULL_NODE_ID};
  VarNodeId input2{NULL_NODE_ID};
  const std::vector<Int> cover{2, 6};
  VarNodeId output1{NULL_NODE_ID};
  VarNodeId output2{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    input1 = retrieveIntVarNode(5, 10, "input1");
    input2 = retrieveIntVarNode(2, 7, "input2");
    output1 = retrieveIntVarNode(1, 2, "output1");
    output2 = retrieveIntVarNode(1, 2, "output2");

    std::vector<VarNodeId> inputVec{input1, input2};

    std::vector<Int> coverVec(cover);

    std::vector<VarNodeId> outputVec{output1, output2};

    createInvariantNode(std::move(inputVec), std::move(coverVec),
                        std::move(outputVec));
  }
};

TEST_F(GlobalCardinalityNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  std::vector<VarNodeId> expectedInputs{input1, input2};
  std::vector<VarNodeId> expectedOutputs{output1, output2};
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
  EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  EXPECT_EQ(invNode().outputVarNodeIds().size(), expectedOutputs.size());
  EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
  EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_F(GlobalCardinalityNodeTest, application) {
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
  EXPECT_EQ(solver.searchVars().size(), 2);
  // input1, input2, output1, output2
  EXPECT_EQ(solver.numVars(), 4);
  // gcc
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(GlobalCardinalityNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }
  EXPECT_EQ(inputVars.size(), 2);

  std::vector<propagation::VarId> outputVars;
  for (const auto& countVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(countVarNodeId), propagation::NULL_ID);
    outputVars.emplace_back(varId(countVarNodeId));
  }
  EXPECT_EQ(outputVars.size(), 2);

  const propagation::VarId violationId =
      invNode().violationVarId(*_invariantGraph);

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);
  std::vector<Int> countVals(outputVars.size());

  std::vector<std::pair<Int, Int>> countBounds;
  countBounds.reserve(outputVars.size());
  for (const propagation::VarId& c : outputVars) {
    countBounds.emplace_back(solver.lowerBound(c), solver.lowerBound(c));
  }

  solver.close();

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(violationId);
    solver.endProbe();

    const std::vector<Int> actual = computeOutputs(inputVals, cover);

    EXPECT_EQ(countVals.size(), outputVars.size());
    for (size_t i = 0; i < countVals.size(); ++i) {
      EXPECT_EQ(solver.currentValue(outputVars.at(i)), actual.at(i));
    }
  }
}

}  // namespace atlantis::testing