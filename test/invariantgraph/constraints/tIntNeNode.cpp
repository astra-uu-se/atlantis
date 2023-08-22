#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/intNeNode.hpp"

static bool isViolating(std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); ++i) {
    for (size_t j = i + 1; j < values.size(); ++j) {
      if (values.at(i) == values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

template <ConstraintType Type>
class AbstractIntNeNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> a;
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::BoolVar> r;

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;

  std::unique_ptr<invariantgraph::IntNeNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = intVar(5, 10, "a");
    b = intVar(2, 7, "b");
    r = boolVar("r");

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "int_ne_reif", std::vector<fznparser::Arg>{fznparser::IntArg{*a},
                                                     fznparser::IntArg{*b},
                                                     fznparser::BoolArg{*r}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_ne", std::vector<fznparser::Arg>{fznparser::IntArg{*a},
                                                  fznparser::IntArg{*b}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_ne_reif", std::vector<fznparser::Arg>{
                               fznparser::IntArg{*a}, fznparser::IntArg{*b},
                               fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_ne_reif", std::vector<fznparser::Arg>{
                               fznparser::IntArg{*a}, fznparser::IntArg{*b},
                               fznparser::BoolArg{true}})));
      }
    }

    node = makeNode<invariantgraph::IntNeNode>(_model->constraints().front());
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

    // notEqual
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

class IntNeNodeTest : public AbstractIntNeNodeTest<ConstraintType::NORMAL> {};

TEST_F(IntNeNodeTest, Construction) { construction(); }

TEST_F(IntNeNodeTest, Application) { application(); }

TEST_F(IntNeNodeTest, Propagation) { propagation(); }

class IntNeReifNodeTest
    : public AbstractIntNeNodeTest<ConstraintType::REIFIED> {};

TEST_F(IntNeReifNodeTest, Construction) { construction(); }

TEST_F(IntNeReifNodeTest, Application) { application(); }

TEST_F(IntNeReifNodeTest, Propagation) { propagation(); }

class IntNeFalseNodeTest
    : public AbstractIntNeNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(IntNeFalseNodeTest, Construction) { construction(); }

TEST_F(IntNeFalseNodeTest, Application) { application(); }

TEST_F(IntNeFalseNodeTest, Propagation) { propagation(); }

class IntNeTrueNodeTest
    : public AbstractIntNeNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(IntNeTrueNodeTest, Construction) { construction(); }

TEST_F(IntNeTrueNodeTest, Application) { application(); }

TEST_F(IntNeTrueNodeTest, Propagation) { propagation(); }