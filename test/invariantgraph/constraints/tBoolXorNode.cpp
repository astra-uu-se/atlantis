#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/boolXorNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  return (values.at(0) == 0) == (values.at(1) == 0);
}

template <ConstraintType Type>
class AbstractBoolXorNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;
  std::unique_ptr<invariantgraph::BoolXorNode> node;

  void SetUp() override {
    if constexpr (Type == ConstraintType::REIFIED) {
      fznparser::Constraint cnstr{"bool_xor", {"a", "b", "r"}, {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::Model mdl{{}, {a, b, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::Model>(std::move(mdl));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        fznparser::Constraint cnstr{"bool_xor", {"a", "b"}, {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        fznparser::Constraint cnstr{"bool_xor", {"a", "b", false}, {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else {
        fznparser::Constraint cnstr{"bool_xor", {"a", "b", true}, {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      }

      fznparser::Model mdl{{}, {a, b}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::Model>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::BoolXorNode>(*constraint);
  }

  void construction() {
    EXPECT_EQ(*node->a()->variable(),
              invariantgraph::VariableNode::FZNVariable(a));
    EXPECT_EQ(*node->b()->variable(),
              invariantgraph::VariableNode::FZNVariable(b));
    expectMarkedAsInput(node.get(), {node->a(), node->b()});
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

    // a, b and the violation
    EXPECT_EQ(engine.numVariables(), 3);

    // BoolXor
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
      engine.updateBounds(inputVariable->varId(), 0, 10, true);
    }

    EXPECT_NE(node->violationVarId(), NULL_ID);
    const VarId violationId = node->violationVarId();

    std::vector<Int> values(inputs.size());
    engine.close();

    for (values.at(0) = 0; values.at(0) <= 1; ++values.at(0)) {
      for (values.at(1) = 0; values.at(1) <= 1; ++values.at(1)) {
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

class BoolXorNodeTest : public AbstractBoolXorNodeTest<ConstraintType::NORMAL> {
};

TEST_F(BoolXorNodeTest, Construction) { construction(); }

TEST_F(BoolXorNodeTest, Application) { application(); }

TEST_F(BoolXorNodeTest, Propagation) { propagation(); }

class BoolXorReifNodeTest
    : public AbstractBoolXorNodeTest<ConstraintType::REIFIED> {};

TEST_F(BoolXorReifNodeTest, Construction) { construction(); }

TEST_F(BoolXorReifNodeTest, Application) { application(); }

TEST_F(BoolXorReifNodeTest, Propagation) { propagation(); }

class BoolXorFalseNodeTest
    : public AbstractBoolXorNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(BoolXorFalseNodeTest, Construction) { construction(); }

TEST_F(BoolXorFalseNodeTest, Application) { application(); }

TEST_F(BoolXorFalseNodeTest, Propagation) { propagation(); }

class BoolXorTrueNodeTest
    : public AbstractBoolXorNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(BoolXorTrueNodeTest, Construction) { construction(); }

TEST_F(BoolXorTrueNodeTest, Application) { application(); }

TEST_F(BoolXorTrueNodeTest, Propagation) { propagation(); }