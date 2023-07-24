#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/intLinEqNode.hpp"

static bool isViolating(const std::vector<Int>& coeffs,
                        const std::vector<Int>& values, const Int expected) {
  Int sum = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    sum += coeffs.at(i) * values.at(i);
  }
  return sum != expected;
}

template <ConstraintType Type>
class AbstractIntLinEqNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 0, 10);
  BOOL_VARIABLE(r);

  Int sum{3};
  std::vector<Int> coeffs{1, 2};

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;
  std::unique_ptr<invariantgraph::IntLinEqNode> node;

  void SetUp() override {
    if constexpr (Type == ConstraintType::REIFIED) {
      fznparser::Constraint cnstr{
          "int_lin_eq_reif",
          {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
           fznparser::Constraint::ArrayArgument{"a", "b"}, sum,
           fznparser::Constraint::Argument{"r"}},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::Model mdl{{}, {a, b, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::Model>(std::move(mdl));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        fznparser::Constraint cnstr{
            "int_lin_eq",
            {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
             fznparser::Constraint::ArrayArgument{"a", "b"}, sum},
            {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        fznparser::Constraint cnstr{
            "int_lin_eq_reif",
            {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
             fznparser::Constraint::ArrayArgument{"a", "b"}, sum, false},
            {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else {
        fznparser::Constraint cnstr{
            "int_lin_eq_reif",
            {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
             fznparser::Constraint::ArrayArgument{"a", "b"}, sum, true},
            {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      }

      fznparser::Model mdl{{}, {a, b}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::Model>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::IntLinEqNode>(*constraint);
  }

  void construction() {
    EXPECT_EQ(*node->staticInputs()[0]->variable(),
              invariantgraph::VariableNode::FZNVariable(a));
    EXPECT_EQ(*node->staticInputs()[1]->variable(),
              invariantgraph::VariableNode::FZNVariable(b));
    expectMarkedAsInput(node.get(), node->staticInputs());

    EXPECT_EQ(node->coeffs()[0], 1);
    EXPECT_EQ(node->coeffs()[1], 2);
    EXPECT_EQ(node->c(), 3);
    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(node->isReified());
      EXPECT_EQ(node->reifiedViolation(), nullptr);
    } else {
      EXPECT_TRUE(node->isReified());
      EXPECT_NE(node->reifiedViolation(), nullptr);
      EXPECT_EQ(node->reifiedViolation()->variable(),
                invariantgraph::VariableNode::FZNVariable(r));
    }
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {a.name, b.name});
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_EQ(definedVariable->varId(), NULL_ID);
    }
    EXPECT_EQ(node->violationVarId(), NULL_ID);
    node->createDefinedVariables(engine);
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_NE(definedVariable->varId(), NULL_ID);
    }
    EXPECT_NE(node->violationVarId(), NULL_ID);
    node->registerWithEngine(engine);
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
    registerVariables(engine, {a.name, b.name});
    node->createDefinedVariables(engine);
    node->registerWithEngine(engine);

    std::vector<VarId> inputs;
    EXPECT_EQ(node->staticInputs().size(), 2);
    for (auto* const inputVariable : node->staticInputs()) {
      EXPECT_NE(inputVariable->varId(), NULL_ID);
      inputs.emplace_back(inputVariable->varId());
    }

    EXPECT_NE(node->violationVarId(), NULL_ID);
    const VarId violationId = node->violationVarId();
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