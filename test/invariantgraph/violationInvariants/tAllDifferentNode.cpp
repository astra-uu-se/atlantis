#include <gmock/gmock.h>

#include <iostream>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "propagation/solver.hpp"

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
  VarNodeId a;
  VarNodeId b;
  VarNodeId c;
  VarNodeId d;
  VarNodeId r;

  InvariantNodeId invNodeId;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(5, 10, "a");
    b = createIntVar(2, 7, "b");
    c = createIntVar(2, 7, "c");
    d = createIntVar(2, 7, "d");
    r = createBoolVar("r");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(a));
    inputs.append(intVar(b));
    inputs.append(intVar(c));
    inputs.append(intVar(d));

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      _model->addConstraint(std::move(
          fznparser::Constraint("fzn_all_different_int_reif",
                                std::vector<fznparser::Arg>{
                                    inputs, fznparser::BoolArg{boolVar(r)}})));
    } else {
      if constexpr (Type == ViolationInvariantType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint{
            "fzn_all_different_int", std::vector<fznparser::Arg>{inputs}}));
      } else if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_all_different_int_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_all_different_int_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
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
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
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

    std::vector<propagation::VarId> inputs;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 4);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);
    EXPECT_EQ(inputs.size(), 4);

    std::vector<Int> values(inputs.size());
    solver.close();

    for (values.at(0) = solver.lowerBound(inputs.at(0));
         values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = solver.lowerBound(inputs.at(1));
           values.at(1) <= solver.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = solver.lowerBound(inputs.at(2));
             values.at(2) <= solver.upperBound(inputs.at(2)); ++values.at(2)) {
          for (values.at(3) = solver.lowerBound(inputs.at(3));
               values.at(3) <= solver.upperBound(inputs.at(3));
               ++values.at(3)) {
            solver.beginMove();
            for (size_t i = 0; i < inputs.size(); ++i) {
              solver.setValue(inputs.at(i), values.at(i));
            }
            solver.endMove();

            solver.beginProbe();
            solver.query(violationId);
            solver.endProbe();
            if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
              EXPECT_EQ(solver.currentValue(violationId) > 0,
                        isViolating(values));
            } else {
              EXPECT_NE(solver.currentValue(violationId) > 0,
                        isViolating(values));
            }
          }
        }
      }
    }
  }
};

class AllDifferentNodeTest
    : public AbstractAllDifferentNodeTest<ViolationInvariantType::NORMAL> {};

TEST_F(AllDifferentNodeTest, Construction) { construction(); }

TEST_F(AllDifferentNodeTest, Application) { application(); }

TEST_F(AllDifferentNodeTest, Propagation) { propagation(); }

class AllDifferentReifNodeTest
    : public AbstractAllDifferentNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(AllDifferentReifNodeTest, Construction) { construction(); }

TEST_F(AllDifferentReifNodeTest, Application) { application(); }

TEST_F(AllDifferentReifNodeTest, Propagation) { propagation(); }

class AllDifferentFalseNodeTest
    : public AbstractAllDifferentNodeTest<ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(AllDifferentFalseNodeTest, Construction) { construction(); }

TEST_F(AllDifferentFalseNodeTest, Application) { application(); }

TEST_F(AllDifferentFalseNodeTest, Propagation) { propagation(); }

class AllDifferentTrueNodeTest
    : public AbstractAllDifferentNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(AllDifferentTrueNodeTest, Construction) { construction(); }

TEST_F(AllDifferentTrueNodeTest, Application) { application(); }

TEST_F(AllDifferentTrueNodeTest, Propagation) { propagation(); }

TEST_F(AllDifferentNodeTest, pruneParameters) {
  fznparser::IntVarArray inputs("");
  inputs.append(7);
  inputs.append(intVar(a));
  inputs.append(10);
  inputs.append(intVar(b));
  inputs.append(6);
  inputs.append(intVar(c));
  inputs.append(9);
  inputs.append(intVar(d));
  inputs.append(5);

  _model->addConstraint(std::move(fznparser::Constraint(
      "fzn_all_different_int", std::vector<fznparser::Arg>{inputs})));

  makeInvNode(_model->constraints().back());

  varNode(b).domain().fix(2);

  EXPECT_TRUE(invNode().prune(*_invariantGraph));

  std::vector<VarNodeId> expectedVars{c, d};

  EXPECT_THAT(expectedVars, ContainerEq(invNode().staticInputVarNodeIds()));

  EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedVars);

  for (const auto& varNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_EQ(varNode(varNodeId).domain().lowerBound(), 3);
    EXPECT_EQ(varNode(varNodeId).domain().upperBound(), 4);
    EXPECT_EQ(varNode(varNodeId).domain().size(), 2);
  }
}
}  // namespace atlantis::testing