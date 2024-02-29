#include <gmock/gmock.h>

#include <iostream>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if (values.at(i) == values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

template <ViolationInvariantType Type>
class AbstractAllDifferentNodeTest : public NodeTestBase<AllDifferentNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId c{NULL_NODE_ID};
  VarNodeId d{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  InvariantNodeId invNodeId;

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
    } else {
      if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE) {
        createInvariantNode(std::move(inputVec), true);
      } else {
        createInvariantNode(std::move(inputVec), false);
      }
    }
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 4);
    std::vector<VarNodeId> expectedVars{a, b, c, d};

    EXPECT_THAT(expectedVars, ContainerEq(invNode().staticInputVarNodeIds()));

    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedVars);

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

class AllDifferentReifNodeTest
    : public AbstractAllDifferentNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(AllDifferentReifNodeTest, Construction) { construction(); }

TEST_F(AllDifferentReifNodeTest, Application) { application(); }

TEST_F(AllDifferentReifNodeTest, Propagation) { propagation(); }

class AllDifferentFalseNodeTest : public AbstractAllDifferentNodeTest<
                                      ViolationInvariantType::CONSTANT_FALSE> {
};

TEST_F(AllDifferentFalseNodeTest, Construction) { construction(); }

TEST_F(AllDifferentFalseNodeTest, Application) { application(); }

TEST_F(AllDifferentFalseNodeTest, Propagation) { propagation(); }

class AllDifferentTrueNodeTest : public AbstractAllDifferentNodeTest<
                                     ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(AllDifferentTrueNodeTest, Construction) { construction(); }

TEST_F(AllDifferentTrueNodeTest, Application) { application(); }

TEST_F(AllDifferentTrueNodeTest, Propagation) { propagation(); }

TEST_F(AllDifferentTrueNodeTest, pruneParameters) {
  std::vector<VarNodeId> inputs{
      retrieveIntVarNode(7), a, retrieveIntVarNode(10), b,
      retrieveIntVarNode(6), c, retrieveIntVarNode(9),  d,
      retrieveIntVarNode(5)};

  const InvariantNodeId invNodeId = _invariantGraph->addInvariantNode(
      std::make_unique<AllDifferentNode>(std::move(inputs), true));

  auto& allDiffNode = dynamic_cast<AllDifferentNode&>(
      _invariantGraph->invariantNode(invNodeId));

  varNode(b).fixValue(Int(2));

  EXPECT_TRUE(allDiffNode.prune(*_invariantGraph));

  std::vector<VarNodeId> expectedVars{c, d};

  EXPECT_THAT(expectedVars, ContainerEq(allDiffNode.staticInputVarNodeIds()));

  EXPECT_EQ(allDiffNode.staticInputVarNodeIds(), expectedVars);

  for (const auto& varNodeId : allDiffNode.staticInputVarNodeIds()) {
    EXPECT_EQ(varNode(varNodeId).lowerBound(), 3);
    EXPECT_EQ(varNode(varNodeId).upperBound(), 4);
    EXPECT_EQ(varNode(varNodeId).constDomain().size(), 2);
  }
}
}  // namespace atlantis::testing
