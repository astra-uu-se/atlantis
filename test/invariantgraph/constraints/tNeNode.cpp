#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/neNode.hpp"

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

template <bool IsReified>
class AbstractNeNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;

  std::unique_ptr<invariantgraph::NeNode> node;

  void SetUp() override {
    if constexpr (!IsReified) {
      fznparser::Constraint cnstr{"int_ne", {"a", "b"}, {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{{}, {a, b}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      fznparser::Constraint cnstr{"int_ne_reif", {"a", "b", "r"}, {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {a, b, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::NeNode>(*constraint);
  }

  void construction() {
    EXPECT_EQ(*node->a()->variable(),
              invariantgraph::VariableNode::FZNVariable(a));
    EXPECT_EQ(*node->b()->variable(),
              invariantgraph::VariableNode::FZNVariable(b));
    expectMarkedAsInput(node.get(), {node->a(), node->b()});

    if constexpr (!IsReified) {
      EXPECT_FALSE(node->isReified());
      EXPECT_NE(node->violation()->variable(),
                invariantgraph::VariableNode::FZNVariable(r));
    } else {
      EXPECT_TRUE(node->isReified());
      EXPECT_EQ(node->violation()->variable(),
                invariantgraph::VariableNode::FZNVariable(r));
    }
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {a.name, b.name});
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_FALSE(_variableMap.contains(definedVariable));
    }
    EXPECT_FALSE(_variableMap.contains(node->violation()));
    node->createDefinedVariables(engine, _variableMap);
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_TRUE(_variableMap.contains(definedVariable));
    }
    EXPECT_TRUE(_variableMap.contains(node->violation()));
    node->registerWithEngine(engine, _variableMap);
    engine.close();

    // a and b
    EXPECT_EQ(engine.searchVariables().size(), 2);

    // a, b and the violation
    EXPECT_EQ(engine.numVariables(), 3);

    // notEqual
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(_variableMap.at(node->violation())), 0);
    EXPECT_EQ(engine.upperBound(_variableMap.at(node->violation())), 1);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {a.name, b.name});
    node->createDefinedVariables(engine, _variableMap);
    node->registerWithEngine(engine, _variableMap);

    std::vector<VarId> inputs;
    EXPECT_EQ(node->staticInputs().size(), 2);
    for (auto* const inputVariable : node->staticInputs()) {
      EXPECT_TRUE(_variableMap.contains(inputVariable));
      inputs.emplace_back(_variableMap.at(inputVariable));
    }

    EXPECT_TRUE(_variableMap.contains(node->violation()));
    const VarId violationId = _variableMap.at(node->violation());
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

        EXPECT_EQ(engine.currentValue(violationId) > 0, isViolating(values));
      }
    }
  }
};

class NeNodeTest : public AbstractNeNodeTest<false> {};

TEST_F(NeNodeTest, Construction) { construction(); }

TEST_F(NeNodeTest, Application) { application(); }

TEST_F(NeNodeTest, Propagation) { propagation(); }

class NeReifNodeTest : public AbstractNeNodeTest<false> {};

TEST_F(NeReifNodeTest, Construction) { construction(); }

TEST_F(NeReifNodeTest, Application) { application(); }

TEST_F(NeReifNodeTest, Propagation) { propagation(); }