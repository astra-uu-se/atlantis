#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/intLinEqNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& coeffs,
                        const std::vector<Int>& values, const Int expected) {
  Int sum = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    sum += coeffs.at(i) * values.at(i);
  }
  return sum != expected;
}

template <ViolationInvariantType Type>
class AbstractIntLinEqNodeTest : public NodeTestBase<IntLinEqNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId r;

  Int sum{3};
  std::vector<Int> coeffs{1, 2};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(0, 10, "a");
    b = createIntVar(0, 10, "b");
    r = createBoolVar("r");

    fznparser::IntVarArray coeffArg("");
    for (const Int coeff : coeffs) {
      coeffArg.append(coeff);
    }

    fznparser::IntVarArray inputArg("");
    inputArg.append(intVar(a));
    inputArg.append(intVar(b));

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "int_lin_eq_reif", {coeffArg, inputArg, fznparser::IntArg{sum},
                              fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ViolationInvariantType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_lin_eq", std::vector<fznparser::Arg>{
                              coeffArg, inputArg, fznparser::IntArg{sum}})));
      } else if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_lin_eq_reif", std::vector<fznparser::Arg>{
                                   coeffArg, inputArg, fznparser::IntArg{sum},
                                   fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_lin_eq_reif", std::vector<fznparser::Arg>{
                                   coeffArg, inputArg, fznparser::IntArg{sum},
                                   fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().at(0), a);
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(1), b);

    EXPECT_EQ(invNode().coeffs().at(0), 1);
    EXPECT_EQ(invNode().coeffs().at(1), 2);
    EXPECT_EQ(invNode().c(), 3);
    if constexpr (Type != ViolationInvariantType::REIFIED) {
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

    // a, b, the linear sum of a and b
    EXPECT_EQ(solver.numVars(), 3);

    // linear
    EXPECT_EQ(solver.numInvariants(), 1);
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

        const Int viol = solver.currentValue(violationId);

        if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
          EXPECT_EQ(viol > 0, isViolating(coeffs, values, sum));
        } else {
          EXPECT_NE(viol > 0, isViolating(coeffs, values, sum));
        }
      }
    }
  }
};

class IntLinEqNodeTest
    : public AbstractIntLinEqNodeTest<ViolationInvariantType::NORMAL> {};

TEST_F(IntLinEqNodeTest, Construction) {}

TEST_F(IntLinEqNodeTest, Application) {}

TEST_F(IntLinEqNodeTest, Propagation) {}

class IntLinEqReifNodeTest
    : public AbstractIntLinEqNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(IntLinEqReifNodeTest, Construction) {}

TEST_F(IntLinEqReifNodeTest, Application) {}

TEST_F(IntLinEqReifNodeTest, Propagation) {}

class IntLinEqFalseNodeTest
    : public AbstractIntLinEqNodeTest<ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(IntLinEqFalseNodeTest, Construction) { construction(); }

TEST_F(IntLinEqFalseNodeTest, Application) { application(); }

TEST_F(IntLinEqFalseNodeTest, Propagation) { propagation(); }

class IntLinEqTrueNodeTest
    : public AbstractIntLinEqNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(IntLinEqTrueNodeTest, Construction) { construction(); }

TEST_F(IntLinEqTrueNodeTest, Application) { application(); }

TEST_F(IntLinEqTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing