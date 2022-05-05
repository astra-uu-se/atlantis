#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/linNeNode.hpp"

static bool isViolating(const std::vector<Int>& coeffs,
                        const std::vector<Int>& values, const Int expected) {
  Int sum = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    sum += coeffs.at(i) * values.at(i);
  }
  return sum == expected;
}

template <bool IsReified>
class AbstractLinNeNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 0, 10);
  BOOL_VARIABLE(r);

  Int sum{3};
  std::vector<Int> coeffs{1, 2};

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;

  std::unique_ptr<invariantgraph::LinNeNode> node;

  void SetUp() override {
    if constexpr (!IsReified) {
      fznparser::Constraint cnstr{
          "int_lin_ne",
          {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
           fznparser::Constraint::ArrayArgument{"a", "b"}, sum},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{{}, {a, b}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      fznparser::Constraint cnstr{
          "int_lin_ne_reif",
          {fznparser::Constraint::ArrayArgument{coeffs.at(0), coeffs.at(1)},
           fznparser::Constraint::ArrayArgument{"a", "b"}, sum,
           fznparser::Constraint::Argument{"r"}},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {a, b, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::LinNeNode>(*constraint);
  }

  void construction() {
    EXPECT_EQ(*node->staticInputs()[0]->variable(),
              invariantgraph::VariableNode::FZNVariable(a));
    EXPECT_EQ(*node->staticInputs()[1]->variable(),
              invariantgraph::VariableNode::FZNVariable(b));
    EXPECT_EQ(node->coeffs()[0], 1);
    EXPECT_EQ(node->coeffs()[1], 2);
    EXPECT_EQ(node->c(), 3);
    expectMarkedAsInput(node.get(), node->staticInputs());

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

    // a, b
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

        const Int viol = engine.currentValue(violationId);

        EXPECT_EQ(viol > 0, isViolating(coeffs, values, sum));
      }
    }
  }
};

class LinNeNodeTest : public AbstractLinNeNodeTest<false> {};

TEST_F(LinNeNodeTest, Construction) { construction(); }

TEST_F(LinNeNodeTest, Application) { application(); }

TEST_F(LinNeNodeTest, Propagation) { propagation(); }

class LinNeReifNodeTest : public AbstractLinNeNodeTest<true> {};

TEST_F(LinNeReifNodeTest, Construction) { construction(); }

TEST_F(LinNeReifNodeTest, Application) { application(); }

TEST_F(LinNeReifNodeTest, Propagation) { propagation(); }