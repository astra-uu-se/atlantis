#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isViolating(const std::vector<Int>& inputVals,
                        const std::vector<Int>& cover,
                        const std::vector<Int>& outputVals) {
  std::vector<Int> counts(cover.size(), 0);
  for (const Int val : inputVals) {
    bool inCover = false;
    for (size_t i = 0; i < cover.size(); ++i) {
      if (val == cover.at(i)) {
        counts.at(i)++;
        inCover = true;
        break;
      }
    }
    if (!inCover) {
      return false;
    }
  }
  for (size_t i = 0; i < outputVals.size(); ++i) {
    if (outputVals.at(i) != counts.at(i)) {
      return false;
    }
  }
  return true;
}

template <ViolationInvariantType Type>
class DISABLED_AbstractGlobalCardinalityClosedNodeTest
    : public NodeTestBase<GlobalCardinalityClosedNode> {
 public:
  VarNodeId x1 = NULL_NODE_ID;
  VarNodeId x2 = NULL_NODE_ID;
  const std::vector<Int> cover{2, 6};
  VarNodeId o1 = NULL_NODE_ID;
  VarNodeId o2 = NULL_NODE_ID;
  VarNodeId reified = NULL_NODE_ID;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVarNode(5, 10, "x1");
    x2 = createIntVarNode(2, 7, "x2");
    o1 = createIntVarNode(1, 2, "o1");
    o2 = createIntVarNode(1, 2, "o2");

    std::vector<VarNodeId> inputVec{x1, x2};

    std::vector<Int> coverVec(cover);
    std::vector<VarNodeId> outputVec{o1, o2};

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      reified = createBoolVarNode("reified", true);
      createInvariantNode(std::move(inputVec), std::move(coverVec),
                          std::move(outputVec), reified);
    } else if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE) {
      createInvariantNode(std::move(inputVec), std::move(coverVec),
                          std::move(outputVec), true);
    } else {
      createInvariantNode(std::move(inputVec), std::move(coverVec),
                          std::move(outputVec), false);
    }
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    std::vector<VarNodeId> expectedInputs{x1, x2};
    std::vector<VarNodeId> expectedOutputs{o1, o2};
    if constexpr (Type == ViolationInvariantType::REIFIED ||
                  Type == ViolationInvariantType::CONSTANT_FALSE) {
      expectedInputs.emplace_back(o1);
      expectedInputs.emplace_back(o2);
      expectedOutputs.clear();
      if constexpr (Type == ViolationInvariantType::REIFIED) {
        expectedOutputs.emplace_back(reified);
      }
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);

    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    EXPECT_EQ(invNode().outputVarNodeIds().size(), expectedOutputs.size());
    EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
    EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), reified);
    } else {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    }
  }

  void application() {
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
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);

    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE ||
                  Type == ViolationInvariantType::CONSTANT_TRUE) {
      // x1, x2
      EXPECT_EQ(solver.searchVars().size(), 2);
      // x1, x2, o1, o2
      // violation
      EXPECT_EQ(solver.numVars(), 5);
      // gcc
      EXPECT_EQ(solver.numInvariants(), 1);
    } else {
      // x1, x2, o1, o2
      EXPECT_EQ(solver.searchVars().size(), 4);
      // x1, x2, o1, o2
      // intermediate o1, intermediate o2
      // 3 intermediate violations
      // 1 total violation
      EXPECT_EQ(solver.numVars(), 10);
      // gcc + 2 (non)-equal + 1 total violation
      EXPECT_EQ(solver.numInvariants(), 4);

      EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)),
                0);
      EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)),
                0);
    }
  }

  void propagation() {
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

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);

    std::vector<Int> inputVals = makeInputVals(solver, inputVars);
    std::vector<Int> outputVals(outputVars.size(), 0);

    while (increaseNextVal(solver, inputVars, inputVals)) {
      solver.beginMove();
      setVarVals(solver, inputVars, inputVals);
      solver.endMove();

      solver.beginProbe();
      solver.query(violationId);
      solver.endProbe();

      updateOutputVals(solver, outputVars, outputVals);

      const Int actual = solver.currentValue(violationId) > 0;
      const Int expected = isViolating(inputVals, cover, outputVals);

      if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
        EXPECT_EQ(actual, expected);
      } else {
        EXPECT_NE(actual, expected);
      }
    }
  }
};

class DISABLED_GlobalCardinalityClosedNodeTest
    : public DISABLED_AbstractGlobalCardinalityClosedNodeTest<
          ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(DISABLED_GlobalCardinalityClosedNodeTest, Construction) {
  construction();
}

TEST_F(DISABLED_GlobalCardinalityClosedNodeTest, Application) { application(); }

TEST_F(DISABLED_GlobalCardinalityClosedNodeTest, Propagation) { propagation(); }

class DISABLED_GlobalCardinalityClosedReifNodeTest
    : public DISABLED_AbstractGlobalCardinalityClosedNodeTest<
          ViolationInvariantType::REIFIED> {};

TEST_F(DISABLED_GlobalCardinalityClosedReifNodeTest, Construction) {
  construction();
}

TEST_F(DISABLED_GlobalCardinalityClosedReifNodeTest, Application) {
  application();
}

TEST_F(DISABLED_GlobalCardinalityClosedReifNodeTest, Propagation) {
  propagation();
}

class DISABLED_GlobalCardinalityClosedFalseNodeTest
    : public DISABLED_AbstractGlobalCardinalityClosedNodeTest<
          ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(DISABLED_GlobalCardinalityClosedFalseNodeTest, Construction) {
  construction();
}

TEST_F(DISABLED_GlobalCardinalityClosedFalseNodeTest, Application) {
  application();
}

TEST_F(DISABLED_GlobalCardinalityClosedFalseNodeTest, Propagation) {
  propagation();
}

class DISABLED_GlobalCardinalityClosedTrueNodeTest
    : public DISABLED_AbstractGlobalCardinalityClosedNodeTest<
          ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(DISABLED_GlobalCardinalityClosedTrueNodeTest, Construction) {
  construction();
}

TEST_F(DISABLED_GlobalCardinalityClosedTrueNodeTest, Application) {
  application();
}

TEST_F(DISABLED_GlobalCardinalityClosedTrueNodeTest, Propagation) {
  propagation();
}

}  // namespace atlantis::testing