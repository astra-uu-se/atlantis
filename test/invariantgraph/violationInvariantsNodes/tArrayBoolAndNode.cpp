#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isViolating(const std::vector<Int>& values) {
  return std::any_of(values.begin(), values.end(),
                     [&](const Int val) { return val > 0; });
}

template <ViolationInvariantType Type>
class AbstractArrayBoolAndNodeTest : public NodeTestBase<ArrayBoolAndNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = retrieveBoolVarNode("a");
    b = retrieveBoolVarNode("b");

    std::vector<VarNodeId> inputs{a, b};

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      reified = retrieveBoolVarNode("reified");
      createInvariantNode(std::move(inputs), reified);
    } else if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE) {
      createInvariantNode(std::move(inputs), reified);
    } else {
      createInvariantNode(std::move(inputs), false);
    }
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
    std::vector<VarNodeId> expectedVars{a, b};
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedVars);
    EXPECT_THAT(expectedVars, ContainerEq(invNode().staticInputVarNodeIds()));
    if constexpr (Type != ViolationInvariantType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), reified);
    }
  }

  void application() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
    }
    invNode().registerOutputVars(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    // a and b
    EXPECT_EQ(solver.searchVars().size(), 2);

    // a, b and reified
    EXPECT_EQ(solver.numVars(), 3);

    // sum
    EXPECT_EQ(solver.numInvariants(), 1);
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVars(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputVars;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVars.emplace_back(varId(inputVarNodeId));
      solver.updateBounds(varId(inputVarNodeId), 0, 10, true);
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);

    solver.close();

    std::vector<Int> inputVals = makeInputVals(solver, inputVars);

    while (increaseNextVal(solver, inputVars, inputVals)) {
      solver.beginMove();
      setVarVals(solver, inputVars, inputVals);
      solver.endMove();

      solver.beginProbe();
      solver.query(violationId);
      solver.endProbe();

      const Int actual = solver.currentValue(violationId) > 0;
      const Int expected = isViolating(inputVals);

      if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
        EXPECT_EQ(actual, expected);
      } else {
        EXPECT_NE(actual, expected);
      }
    }
  }
};

class ArrayBoolAndReifNodeTest
    : public AbstractArrayBoolAndNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(ArrayBoolAndReifNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndReifNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndReifNodeTest, Propagation) { propagation(); }

class ArrayBoolAndFalseNodeTest : public AbstractArrayBoolAndNodeTest<
                                      ViolationInvariantType::CONSTANT_FALSE> {
};

TEST_F(ArrayBoolAndFalseNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndFalseNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndFalseNodeTest, Propagation) { propagation(); }

class ArrayBoolAndTrueNodeTest : public AbstractArrayBoolAndNodeTest<
                                     ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(ArrayBoolAndTrueNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndTrueNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndTrueNodeTest, Propagation) { propagation(); }
}  // namespace atlantis::testing