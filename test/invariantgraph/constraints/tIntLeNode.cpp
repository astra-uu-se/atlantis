#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/intLeNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& values) {
  return values.at(0) > values.at(1);
}

template <ConstraintType Type>
class AbstractIntLeNodeTest : public NodeTestBase<IntLeNode> {
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
          "int_le_reif",
          std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                      fznparser::IntArg{intVar(b)},
                                      fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_le",
            std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                        fznparser::IntArg{intVar(b)}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_le_reif",
            std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                        fznparser::IntArg{intVar(b)},
                                        fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_le_reif",
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
    invNode().registerOutputVariables(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    // a and b
    EXPECT_EQ(solver.searchVariables().size(), 2);

    // a, b and the violation
    EXPECT_EQ(solver.numVariables(), 3);

    // less equal
    EXPECT_EQ(solver.numInvariants(), 1);

    EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVariables(*_invariantGraph, solver);
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

class IntLeNodeTest : public AbstractIntLeNodeTest<ConstraintType::NORMAL> {};

TEST_F(IntLeNodeTest, Construction) { construction(); }

TEST_F(IntLeNodeTest, Application) { application(); }

TEST_F(IntLeNodeTest, Propagation) { propagation(); }

class IntLeReifNodeTest
    : public AbstractIntLeNodeTest<ConstraintType::REIFIED> {};

TEST_F(IntLeReifNodeTest, Construction) { construction(); }

TEST_F(IntLeReifNodeTest, Application) { application(); }

TEST_F(IntLeReifNodeTest, Propagation) { propagation(); }

class IntLeFalseNodeTest
    : public AbstractIntLeNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(IntLeFalseNodeTest, Construction) { construction(); }

TEST_F(IntLeFalseNodeTest, Application) { application(); }

TEST_F(IntLeFalseNodeTest, Propagation) { propagation(); }

class IntLeTrueNodeTest
    : public AbstractIntLeNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(IntLeTrueNodeTest, Construction) { construction(); }

TEST_F(IntLeTrueNodeTest, Application) { application(); }

TEST_F(IntLeTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing