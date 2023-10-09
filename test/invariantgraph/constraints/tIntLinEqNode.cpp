#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/intLinEqNode.hpp"

static bool isViolating(const std::vector<Int>& coeffs,
                        const std::vector<Int>& values, const Int expected) {
  Int sum = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    sum += coeffs.at(i) * values.at(i);
  }
  return sum != expected;
}

template <ConstraintType Type>
class AbstractIntLinEqNodeTest
    : public NodeTestBase<invariantgraph::IntLinEqNode> {
 public:
  invariantgraph::VarNodeId a;
  invariantgraph::VarNodeId b;
  invariantgraph::VarNodeId r;

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

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "int_lin_eq_reif", {coeffArg, inputArg, fznparser::IntArg{sum},
                              fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_lin_eq", std::vector<fznparser::Arg>{
                              coeffArg, inputArg, fznparser::IntArg{sum}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
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
    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(),
                invariantgraph::NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(),
                invariantgraph::NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    }
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), NULL_ID);
    }
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), NULL_ID);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), NULL_ID);
    invNode().registerNode(*_invariantGraph, engine);
    engine.close();

    // a and b
    EXPECT_EQ(engine.searchVariables().size(), 2);

    // a, b, the linear sum of a and b
    EXPECT_EQ(engine.numVariables(), 3);

    // linear
    EXPECT_EQ(engine.numInvariants(), 1);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    invNode().registerNode(*_invariantGraph, engine);

    std::vector<VarId> inputs;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), NULL_ID);
    const VarId violationId = invNode().violationVarId(*_invariantGraph);
    EXPECT_EQ(inputs.size(), 2);

    std::vector<Int> values(inputs.size());
    engine.close();

    for (values.at(0) = engine.lowerBound(inputs.at(0));
         values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = engine.lowerBound(inputs.at(1));
           values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
        engine.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          engine.setValue(inputs.at(i), values.at(i));
        }
        engine.endMove();

        engine.beginProbe();
        engine.query(violationId);
        engine.endProbe();

        const Int viol = engine.currentValue(violationId);

        if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
          EXPECT_EQ(viol > 0, isViolating(coeffs, values, sum));
        } else {
          EXPECT_NE(viol > 0, isViolating(coeffs, values, sum));
        }
      }
    }
  }
};

class IntLinEqNodeTest
    : public AbstractIntLinEqNodeTest<ConstraintType::NORMAL> {};

TEST_F(IntLinEqNodeTest, Construction) {}

TEST_F(IntLinEqNodeTest, Application) {}

TEST_F(IntLinEqNodeTest, Propagation) {}

class IntLinEqReifNodeTest
    : public AbstractIntLinEqNodeTest<ConstraintType::REIFIED> {};

TEST_F(IntLinEqReifNodeTest, Construction) {}

TEST_F(IntLinEqReifNodeTest, Application) {}

TEST_F(IntLinEqReifNodeTest, Propagation) {}

class IntLinEqFalseNodeTest
    : public AbstractIntLinEqNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(IntLinEqFalseNodeTest, Construction) { construction(); }

TEST_F(IntLinEqFalseNodeTest, Application) { application(); }

TEST_F(IntLinEqFalseNodeTest, Propagation) { propagation(); }

class IntLinEqTrueNodeTest
    : public AbstractIntLinEqNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(IntLinEqTrueNodeTest, Construction) { construction(); }

TEST_F(IntLinEqTrueNodeTest, Application) { application(); }

TEST_F(IntLinEqTrueNodeTest, Propagation) { propagation(); }