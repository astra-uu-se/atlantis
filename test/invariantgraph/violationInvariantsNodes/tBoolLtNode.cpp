#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/boolLtNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& values) {
  return values.front() == 0 || values.back() != 0;
}

template <ViolationInvariantType Type>
class AbstractBoolLtNodeTest : public NodeTestBase<BoolLtNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVarNode("a");
    b = createBoolVarNode("b");

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      reified = createBoolVarNode("reified", true);
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
    expectInputTo(invNode());
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

    // equal
    EXPECT_EQ(solver.numInvariants(), 1);

    EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_EQ(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 1);
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
      solver.updateBounds(varId(inputVarNodeId), 0, 5, true);
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

class BoolLtNodeTest
    : public AbstractBoolLtNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(BoolLtNodeTest, Construction) { construction(); }

TEST_F(BoolLtNodeTest, Application) { application(); }

TEST_F(BoolLtNodeTest, Propagation) { propagation(); }

class BoolLtReifNodeTest
    : public AbstractBoolLtNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(BoolLtReifNodeTest, Construction) { construction(); }

TEST_F(BoolLtReifNodeTest, Application) { application(); }

TEST_F(BoolLtReifNodeTest, Propagation) { propagation(); }

class BoolLtFalseNodeTest
    : public AbstractBoolLtNodeTest<ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(BoolLtFalseNodeTest, Construction) { construction(); }

TEST_F(BoolLtFalseNodeTest, Application) { application(); }

TEST_F(BoolLtFalseNodeTest, Propagation) { propagation(); }

class BoolLtTrueNodeTest
    : public AbstractBoolLtNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(BoolLtTrueNodeTest, Construction) { construction(); }

TEST_F(BoolLtTrueNodeTest, Application) { application(); }

TEST_F(BoolLtTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing