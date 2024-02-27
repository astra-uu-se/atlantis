#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/intLtNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& values) {
  return values.at(0) >= values.at(1);
}

template <ViolationInvariantType Type>
class AbstractIntLtNodeTest : public NodeTestBase<IntLtNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = retrieveIntVarNode(5, 10, "a");
    b = retrieveIntVarNode(2, 7, "b");
    if constexpr (Type == ViolationInvariantType::REIFIED) {
      reified = retrieveBoolVarNode("reified");
      createInvariantNode(a, b, reified);
    } else if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE) {
      createInvariantNode(a, b, true);
    } else {
      createInvariantNode(a, b, false);
    }
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().a(), a);
    EXPECT_EQ(invNode().b(), b);

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
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerOutputVars(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    // a and b
    EXPECT_EQ(solver.searchVars().size(), 2);

    // a, b and the violation
    EXPECT_EQ(solver.numVars(), 3);

    // less equal
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

    std::vector<propagation::VarId> inputVars;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVars.emplace_back(varId(inputVarNodeId));
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);
    EXPECT_EQ(inputVars.size(), 2);

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

class IntLtReifNodeTest
    : public AbstractIntLtNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(IntLtReifNodeTest, Construction) { construction(); }

TEST_F(IntLtReifNodeTest, Application) { application(); }

TEST_F(IntLtReifNodeTest, Propagation) { propagation(); }

class IntLinLtFalseNodeTest
    : public AbstractIntLtNodeTest<ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(IntLinLtFalseNodeTest, Construction) { construction(); }

TEST_F(IntLinLtFalseNodeTest, Application) { application(); }

TEST_F(IntLinLtFalseNodeTest, Propagation) { propagation(); }

class IntLinLtTrueNodeTest
    : public AbstractIntLtNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(IntLinLtTrueNodeTest, Construction) { construction(); }

TEST_F(IntLinLtTrueNodeTest, Application) { application(); }

TEST_F(IntLinLtTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing