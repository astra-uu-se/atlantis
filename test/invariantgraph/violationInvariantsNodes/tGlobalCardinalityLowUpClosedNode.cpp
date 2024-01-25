#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/globalCardinalityLowUpClosedNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isSatisfied(const std::vector<Int>& values,
                        const std::vector<Int>& cover,
                        const std::vector<Int>& low,
                        const std::vector<Int>& up) {
  std::vector<Int> counts(cover.size(), 0);
  bool sat = true;
  for (const Int val : values) {
    bool inCover = false;
    for (size_t i = 0; i < cover.size(); ++i) {
      if (val == cover.at(i)) {
        counts.at(i)++;
        inCover = true;
        break;
      }
    }
    sat = sat && inCover;
  }
  for (size_t i = 0; i < counts.size(); ++i) {
    sat = sat && low.at(i) <= counts.at(i) && counts.at(i) <= up.at(i);
  }
  return sat;
}

template <ViolationInvariantType Type>
class DISABLED_AbstractGlobalCardinalityLowUpClosedNodeTest
    : public NodeTestBase<GlobalCardinalityLowUpClosedNode> {
 public:
  VarNodeId x1 = NULL_NODE_ID;
  VarNodeId x2 = NULL_NODE_ID;
  const std::vector<Int> cover{2, 6};
  const std::vector<Int> low{0, 1};
  const std::vector<Int> up{1, 2};
  VarNodeId reified = NULL_NODE_ID;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVarNode(5, 10, "x1");
    x2 = createIntVarNode(2, 7, "x2");
    reified = createBoolVarNode("reified", true);

    std::vector<VarNodeId> inputVec{x1, x2};

    std::vector<Int> coverVec(cover);

    std::vector<Int> lowVec(low);

    std::vector<Int> upVec(up);

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      reified = createBoolVarNode("reified", true);
      createInvariantNode(std::move(inputVec), std::move(coverVec),
                          std::move(lowVec), std::move(upVec), reified);
    } else if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE) {
      createInvariantNode(std::move(inputVec), std::move(coverVec),
                          std::move(lowVec), std::move(upVec), true);
    } else {
      createInvariantNode(std::move(inputVec), std::move(coverVec),
                          std::move(lowVec), std::move(upVec), false);
    }
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    const size_t numInputs = 2;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numInputs);
    std::vector<VarNodeId> expectedInputs{x1, x2};
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    if (Type == ViolationInvariantType::REIFIED) {
      EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
      EXPECT_EQ(invNode().outputVarNodeIds().front(), reified);
    } else {
      EXPECT_EQ(invNode().outputVarNodeIds().size(), 0);
    }

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

    // x1, x2
    EXPECT_EQ(solver.searchVars().size(), 2);
    // x1, x2, violation
    // violation
    EXPECT_EQ(solver.numVars(), 3);
    // gcc
    EXPECT_EQ(solver.numInvariants(), 1);
    EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVars(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputs;
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }
    EXPECT_EQ(inputs.size(), 2);

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);

    std::vector<Int> inputVals(inputs.size());

    solver.close();

    for (inputVals.at(0) = solver.lowerBound(inputs.at(0));
         inputVals.at(0) <= solver.upperBound(inputs.at(0));
         ++inputVals.at(0)) {
      for (inputVals.at(1) = solver.lowerBound(inputs.at(1));
           inputVals.at(1) <= solver.upperBound(inputs.at(1));
           ++inputVals.at(1)) {
        solver.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          solver.setValue(inputs.at(i), inputVals.at(i));
        }
        solver.endMove();

        solver.beginProbe();
        solver.query(violationId);
        solver.endProbe();

        if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
          bool sat = isSatisfied(inputVals, cover, low, up);
          EXPECT_EQ(solver.currentValue(violationId) == 0, sat);
        } else {
          bool sat = isSatisfied(inputVals, cover, low, up);
          EXPECT_NE(solver.currentValue(violationId) == 0, sat);
        }
      }
    }
  }
};

class DISABLED_GlobalCardinalityLowUpClosedNodeTest
    : public DISABLED_AbstractGlobalCardinalityLowUpClosedNodeTest<
          ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(DISABLED_GlobalCardinalityLowUpClosedNodeTest, Construction) {
  construction();
}

TEST_F(DISABLED_GlobalCardinalityLowUpClosedNodeTest, Application) {
  application();
}

TEST_F(DISABLED_GlobalCardinalityLowUpClosedNodeTest, Propagation) {
  propagation();
}

class DISABLED_GlobalCardinalityLowUpClosedReifNodeTest
    : public DISABLED_AbstractGlobalCardinalityLowUpClosedNodeTest<
          ViolationInvariantType::REIFIED> {};

TEST_F(DISABLED_GlobalCardinalityLowUpClosedReifNodeTest, Construction) {
  construction();
}

TEST_F(DISABLED_GlobalCardinalityLowUpClosedReifNodeTest, Application) {
  application();
}

TEST_F(DISABLED_GlobalCardinalityLowUpClosedReifNodeTest, Propagation) {
  propagation();
}

class DISABLED_GlobalCardinalityLowUpClosedFalseNodeTest
    : public DISABLED_AbstractGlobalCardinalityLowUpClosedNodeTest<
          ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(DISABLED_GlobalCardinalityLowUpClosedFalseNodeTest, Construction) {
  construction();
}

TEST_F(DISABLED_GlobalCardinalityLowUpClosedFalseNodeTest, Application) {
  application();
}

TEST_F(DISABLED_GlobalCardinalityLowUpClosedFalseNodeTest, Propagation) {
  propagation();
}

class DISABLED_GlobalCardinalityLowUpClosedTrueNodeTest
    : public DISABLED_AbstractGlobalCardinalityLowUpClosedNodeTest<
          ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(DISABLED_GlobalCardinalityLowUpClosedTrueNodeTest, Construction) {
  construction();
}

TEST_F(DISABLED_GlobalCardinalityLowUpClosedTrueNodeTest, Application) {
  application();
}

TEST_F(DISABLED_GlobalCardinalityLowUpClosedTrueNodeTest, Propagation) {
  propagation();
}

}  // namespace atlantis::testing