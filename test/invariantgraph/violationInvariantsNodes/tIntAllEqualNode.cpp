#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if (values.at(i) != values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

template <ViolationInvariantType Type>
class AbstractAllEqualNodeTest : public NodeTestBase<IntAllEqualNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId c{NULL_NODE_ID};
  VarNodeId d{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = retrieveIntVarNode(5, 10, "a");
    b = retrieveIntVarNode(2, 7, "b");
    c = retrieveIntVarNode(2, 7, "c");
    d = retrieveIntVarNode(2, 7, "d");

    std::vector<VarNodeId> inputVec{a, b, c, d};
    if constexpr (Type == ViolationInvariantType::REIFIED) {
      reified = retrieveBoolVarNode("reified");
      createInvariantNode(std::move(inputVec), reified);
    } else if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE) {
      createInvariantNode(std::move(inputVec), true);
    } else {
      createInvariantNode(std::move(inputVec), false);
    }
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 4);
    std::vector<VarNodeId> expectedVars{a, b, c, d};
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
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerOutputVars(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    // a, b, c and d
    EXPECT_EQ(solver.searchVars().size(), 4);

    // a, b, c, d and the violation
    EXPECT_EQ(solver.numVars(), 5);

    // alldifferent
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
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 4);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVars.emplace_back(varId(inputVarNodeId));
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);
    EXPECT_EQ(inputVars.size(), 4);

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

class AllEqualReifNodeTest
    : public AbstractAllEqualNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(AllEqualReifNodeTest, Construction) { construction(); }

TEST_F(AllEqualReifNodeTest, Application) { application(); }

TEST_F(AllEqualReifNodeTest, Propagation) { propagation(); }

class AllEqualFalseNodeTest
    : public AbstractAllEqualNodeTest<ViolationInvariantType::CONSTANT_FALSE> {
};

TEST_F(AllEqualFalseNodeTest, Construction) { construction(); }

TEST_F(AllEqualFalseNodeTest, Application) { application(); }

TEST_F(AllEqualFalseNodeTest, Propagation) { propagation(); }

class AllEqualTrueNodeTest
    : public AbstractAllEqualNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(AllEqualTrueNodeTest, Construction) { construction(); }

TEST_F(AllEqualTrueNodeTest, Application) { application(); }

TEST_F(AllEqualTrueNodeTest, Propagation) { propagation(); }
}  // namespace atlantis::testing