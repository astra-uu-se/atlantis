#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/intLeNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  return values.at(0) > values.at(1);
}

template <ConstraintType Type>
class AbstractIntLeNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> a;
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::BoolVar> r;

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;
  std::unique_ptr<invariantgraph::IntLeNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = intVar(5, 10, "a");
    b = intVar(2, 7, "b");
    r = boolVar("r");

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "int_le_reif", std::vector<fznparser::Arg>{fznparser::IntArg{*a},
                                                     fznparser::IntArg{*b},
                                                     fznparser::BoolArg{*r}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_le", std::vector<fznparser::Arg>{fznparser::IntArg{*a},
                                                  fznparser::IntArg{*b}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_le_reif", std::vector<fznparser::Arg>{
                               fznparser::IntArg{*a}, fznparser::IntArg{*b},
                               fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_le_reif", std::vector<fznparser::Arg>{
                               fznparser::IntArg{*a}, fznparser::IntArg{*b},
                               fznparser::BoolArg{true}})));
      }
    }

    node = makeNode<invariantgraph::IntLeNode>(_model->constraints().front());
  }

  void construction() {
    EXPECT_EQ(*node->a()->variable(), invariantgraph::VarNode::FZNVariable(*a));
    EXPECT_EQ(*node->b()->variable(), invariantgraph::VarNode::FZNVariable(*b));
    expectMarkedAsInput(node.get(), {node->a(), node->b()});
    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(node->isReified());
      EXPECT_EQ(node->reifiedViolation(), nullptr);
    } else {
      EXPECT_TRUE(node->isReified());
      EXPECT_NE(node->reifiedViolation(), nullptr);
      EXPECT_EQ(node->reifiedViolation()->variable(),
                invariantgraph::VarNode::FZNVariable(*r));
    }
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    addVariablesToEngine(engine);
    for (auto* const definedVariable : node->outputVarNodeIds()) {
      EXPECT_EQ(definedVariable->varId(), NULL_ID);
    }
    EXPECT_EQ(node->violationVarId(), NULL_ID);
    node->registerOutputVariables(engine);
    for (auto* const definedVariable : node->outputVarNodeIds()) {
      EXPECT_NE(definedVariable->varId(), NULL_ID);
    }
    EXPECT_NE(node->violationVarId(), NULL_ID);
    node->registerNode(*_invariantGraph, engine);
    engine.close();

    // a and b
    EXPECT_EQ(engine.searchVariables().size(), 2);

    // a, b and the violation
    EXPECT_EQ(engine.numVariables(), 3);

    // less equal
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(node->violationVarId()), 0);
    EXPECT_GT(engine.upperBound(node->violationVarId()), 0);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    addVariablesToEngine(engine);
    node->registerOutputVariables(engine);
    node->registerNode(*_invariantGraph, engine);

    std::vector<VarId> inputs;
    EXPECT_EQ(node->staticInputVarNodeIds().size(), 2);
    for (auto* const inputVariable : node->staticInputVarNodeIds()) {
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

        if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
          EXPECT_EQ(engine.currentValue(violationId) > 0, isViolating(values));
        } else {
          EXPECT_NE(engine.currentValue(violationId) > 0, isViolating(values));
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