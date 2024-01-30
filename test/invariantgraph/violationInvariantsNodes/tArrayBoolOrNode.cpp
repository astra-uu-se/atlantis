#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isViolating(const std::vector<Int>& values) {
  return std::all_of(values.begin(), values.end(), [&](const Int val) {
    return val > 0;
  });
}

template <ViolationInvariantType Type>
class AbstractArrayBoolOrNodeTest : public NodeTestBase<ArrayBoolOrNode> {
 public:
  VarNodeId x1{NULL_NODE_ID};
  VarNodeId x2{NULL_NODE_ID};
  VarNodeId x3{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createBoolVarNode("x1");
    x2 = createBoolVarNode("x2");
    x3 = createBoolVarNode("x3");

    std::vector<VarNodeId> inputs{x1, x2, x3};

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      reified = createBoolVarNode("reified", true);
      createInvariantNode(std::move(inputs), reified);
    } else if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE) {
      createInvariantNode(std::move(inputs), true);
    } else {
      createInvariantNode(std::move(inputs), false);
    }
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
    EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
    std::vector<VarNodeId> expectedVars{x1, x2, x3};
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedVars);
    EXPECT_THAT(expectedVars, ContainerEq(invNode().staticInputVarNodeIds()));
    if constexpr (Type == ViolationInvariantType::REIFIED) {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    } else {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
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

    // x1, x2, and x3
    EXPECT_EQ(solver.searchVars().size(), 3);

    // x1, x2, x3 and reified
    EXPECT_EQ(solver.numVars(), 4);

    // minSparse
    EXPECT_EQ(solver.numInvariants(), 1);
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVars(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputVars;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
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

class ArrayBoolOrNodeTest : public AbstractArrayBoolOrNodeTest<
                                ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(ArrayBoolOrNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrNodeTest, Propagation) { propagation(); }

class ArrayBoolOrReifNodeTest
    : public AbstractArrayBoolOrNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(ArrayBoolOrReifNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrReifNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrReifNodeTest, Propagation) { propagation(); }

class ArrayBoolOrFalseNodeTest : public AbstractArrayBoolOrNodeTest<
                                     ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(ArrayBoolOrFalseNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrFalseNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrFalseNodeTest, Propagation) { propagation(); }

class ArrayBoolOrTrueNodeTest : public AbstractArrayBoolOrNodeTest<
                                    ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(ArrayBoolOrTrueNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrTrueNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing