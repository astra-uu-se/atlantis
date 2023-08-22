#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if ((values.at(i) == 0) != (values.at(j) == 0)) {
        return true;
      }
    }
  }
  return false;
}

template <ConstraintType Type>
class AbstractBoolEqNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::BoolVar> a;
  std::unique_ptr<fznparser::BoolVar> b;
  std::unique_ptr<fznparser::BoolVar> r;

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;
  std::unique_ptr<invariantgraph::BoolEqNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = boolVar("a");
    b = boolVar("b");
    r = boolVar("r");

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "bool_eq_reif", std::vector<fznparser::Arg>{
                              fznparser::BoolArg{*a}, fznparser::BoolArg{*b},
                              fznparser::BoolArg{*r}})));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_eq", std::vector<fznparser::Arg>{fznparser::BoolArg{*a},
                                                   fznparser::BoolArg{*b}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_eq_reif", std::vector<fznparser::Arg>{
                                fznparser::BoolArg{*a}, fznparser::BoolArg{*b},
                                fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_eq_reif", std::vector<fznparser::Arg>{
                                fznparser::BoolArg{*a}, fznparser::BoolArg{*b},
                                fznparser::BoolArg{true}})));
      }
    }

    node = makeNode<invariantgraph::BoolEqNode>(_model->constraints().front());
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

    // equal
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(node->violationVarId()), 0);
    EXPECT_EQ(engine.upperBound(node->violationVarId()), 1);
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
      engine.updateBounds(inputVariable->varId(), 0, 5, true);
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

class BoolEqNodeTest : public AbstractBoolEqNodeTest<ConstraintType::NORMAL> {};

TEST_F(BoolEqNodeTest, Construction) { construction(); }

TEST_F(BoolEqNodeTest, Application) { application(); }

TEST_F(BoolEqNodeTest, Propagation) { propagation(); }

class BoolEqReifNodeTest
    : public AbstractBoolEqNodeTest<ConstraintType::REIFIED> {};

TEST_F(BoolEqReifNodeTest, Construction) { construction(); }

TEST_F(BoolEqReifNodeTest, Application) { application(); }

TEST_F(BoolEqReifNodeTest, Propagation) { propagation(); }

class BoolEqFalseNodeTest
    : public AbstractBoolEqNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(BoolEqFalseNodeTest, Construction) { construction(); }

TEST_F(BoolEqFalseNodeTest, Application) { application(); }

TEST_F(BoolEqFalseNodeTest, Propagation) { propagation(); }

class BoolEqTrueNodeTest
    : public AbstractBoolEqNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(BoolEqTrueNodeTest, Construction) { construction(); }

TEST_F(BoolEqTrueNodeTest, Application) { application(); }

TEST_F(BoolEqTrueNodeTest, Propagation) { propagation(); }