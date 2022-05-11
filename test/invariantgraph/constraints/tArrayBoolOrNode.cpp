#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/arrayBoolOrNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  for (const Int val : values) {
    if (val == 0) {
      return false;
    }
  }
  return true;
}

template <bool IsReified>
class AbstractArrayBoolOrNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;
  std::unique_ptr<invariantgraph::ArrayBoolOrNode> node;

  void SetUp() override {
    if constexpr (!IsReified) {
      fznparser::Constraint cnstr{
          "array_bool_or",
          {fznparser::Constraint::ArrayArgument{"a", "b"},
           fznparser::Constraint::Argument{true}},
          {}};

TEST_F(ArrayBoolOrNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
            invariantgraph::VariableNode::FZNVariable(r));

      fznparser::FZNModel mdl{{}, {a, b}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      fznparser::Constraint cnstr{
          "array_bool_or",
          {fznparser::Constraint::ArrayArgument{"a", "b"},
           fznparser::Constraint::Argument{"r"}},
          {}};

TEST_F(ArrayBoolOrNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->createDefinedVariables(engine);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerWithEngine(engine);
  engine.close();

      fznparser::FZNModel mdl{
          {}, {a, b, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::ArrayBoolOrNode>(*constraint);
  }

  void construction() {
    EXPECT_EQ(node->staticInputs().size(), 2);
    EXPECT_EQ(node->dynamicInputs().size(), 0);
    std::vector<invariantgraph::VariableNode*> expectedVars;
    for (size_t i = 0; i < 2; ++i) {
      expectedVars.emplace_back(_variables.at(i).get());
    }
    EXPECT_EQ(node->staticInputs(), expectedVars);
    EXPECT_THAT(expectedVars, testing::ContainerEq(node->staticInputs()));
    expectMarkedAsInput(node.get(), node->staticInputs());
    if constexpr (!IsReified) {
      EXPECT_FALSE(node->isReified());
      EXPECT_EQ(node->reifiedViolation(), nullptr);
    } else {
      EXPECT_TRUE(node->isReified());
      EXPECT_NE(node->reifiedViolation(), nullptr);
      EXPECT_EQ(node->reifiedViolation()->variable(),
                invariantgraph::VariableNode::FZNVariable(r));
    }
  }

TEST_F(ArrayBoolOrNodeTest, propagation) {
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

  EXPECT_NE(node->definedVariables().front()->varId(), NULL_ID);
  const VarId outputId = node->definedVariables().front()->varId();
  EXPECT_EQ(inputs.size(), 2);

    std::vector<VarId> inputs;
    EXPECT_EQ(node->staticInputs().size(), 2);
    for (auto* const inputVariable : node->staticInputs()) {
      EXPECT_NE(inputVariable->varId(), NULL_ID);
      inputs.emplace_back(inputVariable->varId());
    }

    EXPECT_NE(node->violationVarId(), NULL_ID);
    const VarId outputId = node->violationVarId();
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
        engine.query(outputId);
        engine.endProbe();

        const Int viol = engine.currentValue(outputId);

        EXPECT_EQ(viol > 0, isViolating(values));
      }
    }
  }
};

class ArrayBoolOrNodeTest : public AbstractArrayBoolOrNodeTest<false> {};

TEST_F(ArrayBoolOrNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrNodeTest, Propagation) { propagation(); }

class ArrayBoolOrReifNodeTest : public AbstractArrayBoolOrNodeTest<true> {};

TEST_F(ArrayBoolOrReifNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrReifNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrReifNodeTest, Propagation) { propagation(); }

TEST_F(ArrayBoolOrReifNodeTest, r_is_a_constant) {
  _nodeMap.clear();
  _variables.clear();

  fznparser::Constraint constraint2{
      "array_bool_or",
      {fznparser::Constraint::ArrayArgument{a.name, b.name}, true},
      {}};

  node = makeNode<invariantgraph::ArrayBoolOrNode>(constraint2);
  EXPECT_TRUE(!node->isReified());
  EXPECT_FALSE(!node->definedVariables().empty());

  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->createDefinedVariables(engine);
  node->registerWithEngine(engine);
  engine.close();

  engine.beginMove();
  engine.setValue(engineVariable(a), 0);
  engine.setValue(engineVariable(b), 0);
  engine.endMove();

  engine.beginCommit();
  engine.query(node->violationVarId());
  engine.endCommit();
  EXPECT_EQ(engine.committedValue(node->violationVarId()), 0);

  engine.beginMove();
  engine.setValue(engineVariable(a), 1);
  engine.setValue(engineVariable(b), 0);
  engine.endMove();

  engine.beginCommit();
  engine.query(node->violationVarId());
  engine.endCommit();
  EXPECT_EQ(engine.committedValue(node->violationVarId()), 0);

  engine.beginMove();
  engine.setValue(engineVariable(a), 1);
  engine.setValue(engineVariable(b), 1);
  engine.endMove();

  engine.beginCommit();
  engine.query(node->violationVarId());
  engine.endCommit();
  EXPECT_GT(engine.committedValue(node->violationVarId()), 0);
}
