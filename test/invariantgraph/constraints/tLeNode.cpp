#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/leNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  return values.at(0) > values.at(1);
}

template <bool IsReified>
class AbstractLeNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;
  std::unique_ptr<invariantgraph::LeNode> node;

  void SetUp() override {
    if constexpr (!IsReified) {
      fznparser::Constraint cnstr{"int_le", {"a", "b"}, {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{{}, {a, b}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      fznparser::Constraint cnstr{"int_le_reif", {"a", "b", "r"}, {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {a, b, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::LeNode>(*constraint);
  }

  void construction() {
    EXPECT_EQ(*node->a()->variable(),
              invariantgraph::VariableNode::FZNVariable(a));
    EXPECT_EQ(*node->b()->variable(),
              invariantgraph::VariableNode::FZNVariable(b));
    expectMarkedAsInput(node.get(), {node->a(), node->b()});
    if constexpr (IsReified) {
      EXPECT_TRUE(node->isReified());
    } else {
      EXPECT_FALSE(node->isReified());
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

    // less equal
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(_variableMap.at(node->violation())), 0);
    EXPECT_EQ(engine.upperBound(_variableMap.at(node->violation())), 8);
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

class LeNodeTest : public AbstractLeNodeTest<false> {};

TEST_F(LeNodeTest, Construction) { construction(); }

TEST_F(LeNodeTest, Application) { application(); }

TEST_F(LeNodeTest, Propagation) { propagation(); }

class LeReifNodeTest : public AbstractLeNodeTest<true> {};

TEST_F(LeReifNodeTest, Construction) { construction(); }

TEST_F(LeReifNodeTest, Application) { application(); }

TEST_F(LeReifNodeTest, Propagation) { propagation(); }