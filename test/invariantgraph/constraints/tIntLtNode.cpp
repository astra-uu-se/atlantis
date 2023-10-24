#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/intLtNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& values) {
  return values.at(0) >= values.at(1);
}

template <ConstraintType Type>
class AbstractIntLtNodeTest : public NodeTestBase<IntLtNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(5, 10, "a");
    b = createIntVar(2, 7, "b");
    r = createBoolVar("r");

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "int_lt_reif",
          std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                      fznparser::IntArg{intVar(b)},
                                      fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_lt",
            std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                        fznparser::IntArg{intVar(b)}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_lt_reif",
            std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                        fznparser::IntArg{intVar(b)},
                                        fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_lt_reif",
            std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                        fznparser::IntArg{intVar(b)},
                                        fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().a(), a);
    EXPECT_EQ(invNode().b(), b);

    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
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

    std::vector<propagation::VarId> inputs;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId = invNode().violationVarId(*_invariantGraph);
    EXPECT_EQ(inputs.size(), 2);

    std::vector<Int> values(inputs.size());
    solver.close();

    for (values.at(0) = solver.lowerBound(inputs.at(0));
         values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = solver.lowerBound(inputs.at(1));
           values.at(1) <= solver.upperBound(inputs.at(1)); ++values.at(1)) {
        solver.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          solver.setValue(inputs.at(i), values.at(i));
        }
        solver.endMove();

        solver.beginProbe();
        solver.query(violationId);
        solver.endProbe();

        if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
          EXPECT_EQ(solver.currentValue(violationId) > 0, isViolating(values));
        } else {
          EXPECT_NE(solver.currentValue(violationId) > 0, isViolating(values));
        }
      }
    }
  }
};

class IntLtNodeTest : public AbstractIntLtNodeTest<ConstraintType::NORMAL> {};

TEST_F(IntLtNodeTest, Construction) { construction(); }

TEST_F(IntLtNodeTest, Application) { application(); }

TEST_F(IntLtNodeTest, Propagation) { propagation(); }

class IntLtReifNodeTest
    : public AbstractIntLtNodeTest<ConstraintType::REIFIED> {};

TEST_F(IntLtReifNodeTest, Construction) { construction(); }

TEST_F(IntLtReifNodeTest, Application) { application(); }

TEST_F(IntLtReifNodeTest, Propagation) { propagation(); }

class IntLinLtFalseNodeTest
    : public AbstractIntLtNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(IntLinLtFalseNodeTest, Construction) { construction(); }

TEST_F(IntLinLtFalseNodeTest, Application) { application(); }

TEST_F(IntLinLtFalseNodeTest, Propagation) { propagation(); }

class IntLinLtTrueNodeTest
    : public AbstractIntLtNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(IntLinLtTrueNodeTest, Construction) { construction(); }

TEST_F(IntLinLtTrueNodeTest, Application) { application(); }

TEST_F(IntLinLtTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing