#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/intLinLeNode.hpp"

static bool isViolating(const std::vector<Int>& coeffs,
                        const std::vector<Int>& values, const Int expected) {
  Int sum = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    sum += coeffs.at(i) * values.at(i);
  }
  return sum > expected;
}

template <ConstraintType Type>
class AbstractIntLinLeNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 0, 10);
  BOOL_VARIABLE(r);

  Int sum{3};
  std::vector<Int> coeffs{1, 2};

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;

  std::unique_ptr<invariantgraph::IntLinLeNode> node;

  void SetUp() override {
    if constexpr (Type == ConstraintType::REIFIED) {
      fznparser::Constraint cnstr{
          "int_lin_le_reif",
          {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
           fznparser::Constraint::ArrayArgument{"a", "b"}, sum,
           fznparser::Constraint::Argument{"r"}},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {a, b, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        fznparser::Constraint cnstr{
            "int_lin_le",
            {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
             fznparser::Constraint::ArrayArgument{"a", "b"}, sum},
            {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        fznparser::Constraint cnstr{
            "int_lin_le_reif",
            {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
             fznparser::Constraint::ArrayArgument{"a", "b"}, sum, false},
            {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else {
        fznparser::Constraint cnstr{
            "int_lin_le_reif",
            {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
             fznparser::Constraint::ArrayArgument{"a", "b"}, sum, true},
            {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      }

      fznparser::FZNModel mdl{{}, {a, b}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::IntLinLeNode>(*constraint);
  }

  void construction() {
    EXPECT_EQ(*node->staticInputs()[0]->variable(),
              invariantgraph::VariableNode::FZNVariable(a));
    EXPECT_EQ(*node->staticInputs()[1]->variable(),
              invariantgraph::VariableNode::FZNVariable(b));
    EXPECT_EQ(node->coeffs()[0], 1);
    EXPECT_EQ(node->coeffs()[1], 2);
    EXPECT_EQ(node->bound(), 3);
    expectMarkedAsInput(node.get(), node->staticInputs());
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

    // a, b
    EXPECT_EQ(engine.searchVariables().size(), 2);

    // a, b, the linear sum of a and b
    EXPECT_EQ(engine.numVariables(), 3);

    // linear
    EXPECT_EQ(engine.numInvariants(), 1);

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

class IntLinLeNodeTest
    : public AbstractIntLinLeNodeTest<ConstraintType::NORMAL> {};

TEST_F(IntLinLeNodeTest, Construction) { construction(); }

TEST_F(IntLinLeNodeTest, Application) { application(); }

TEST_F(IntLinLeNodeTest, Propagation) { propagation(); }

class IntLinLeReifNodeTest
    : public AbstractIntLinLeNodeTest<ConstraintType::REIFIED> {};

TEST_F(IntLinLeReifNodeTest, Construction) { construction(); }

TEST_F(IntLinLeReifNodeTest, Application) { application(); }

TEST_F(IntLinLeReifNodeTest, Propagation) { propagation(); }

class IntLinLeFalseNodeTest
    : public AbstractIntLinLeNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(IntLinLeFalseNodeTest, Construction) { construction(); }

TEST_F(IntLinLeFalseNodeTest, Application) { application(); }

TEST_F(IntLinLeFalseNodeTest, Propagation) { propagation(); }

class IntLinLeTrueNodeTest
    : public AbstractIntLinLeNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(IntLinLeTrueNodeTest, Construction) { construction(); }

TEST_F(IntLinLeTrueNodeTest, Application) { application(); }

TEST_F(IntLinLeTrueNodeTest, Propagation) { propagation(); }