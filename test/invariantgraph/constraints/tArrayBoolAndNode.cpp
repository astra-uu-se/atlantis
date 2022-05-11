#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/arrayBoolAndNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  for (const Int val : values) {
    if (val > 0) {
      return true;
    }
  }
  return false;
}

template <bool IsReified>
class AbstractArrayBoolAndNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;
  std::unique_ptr<invariantgraph::ArrayBoolAndNode> node;

  void SetUp() override {
    if constexpr (!IsReified) {
      fznparser::Constraint cnstr{
          "array_bool_and",
          {fznparser::Constraint::ArrayArgument{"a", "b"},
           fznparser::Constraint::Argument{true}},
          {}};

TEST_F(ArrayBoolAndNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
            invariantgraph::VariableNode::FZNVariable(r));

      fznparser::FZNModel mdl{{}, {a, b}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      fznparser::Constraint cnstr{
          "array_bool_and",
          {fznparser::Constraint::ArrayArgument{"a", "b"},
           fznparser::Constraint::Argument{"r"}},
          {}};

TEST_F(ArrayBoolAndNodeTest, application) {
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

  void application() {
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

    // a and b
    EXPECT_EQ(engine.searchVariables().size(), 2);

    // a, b and r
    EXPECT_EQ(engine.numVariables(), 3);

    // sum
    EXPECT_EQ(engine.numInvariants(), 1);
  }

TEST_F(ArrayBoolAndNodeTest, propagation) {
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

class ArrayBoolAndNodeTest : public AbstractArrayBoolAndNodeTest<false> {};

TEST_F(ArrayBoolAndNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndNodeTest, Propagation) { propagation(); }

class ArrayBoolAndReifNodeTest : public AbstractArrayBoolAndNodeTest<true> {};

TEST_F(ArrayBoolAndReifNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndReifNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndReifNodeTest, Propagation) { propagation(); }

TEST_F(ArrayBoolAndReifNodeTest, r_is_a_constant) {
  _nodeMap.clear();
  _variables.clear();

  fznparser::Constraint constraint2{
      "array_bool_and",
      {fznparser::Constraint::ArrayArgument{a.name, b.name}, true},
      {}};

  node = makeNode<invariantgraph::ArrayBoolAndNode>(constraint2);
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
  EXPECT_GT(engine.committedValue(node->violationVarId()), 0);

  engine.beginMove();
  engine.setValue(engineVariable(a), 1);
  engine.setValue(engineVariable(b), 1);
  engine.endMove();

  engine.beginCommit();
  engine.query(node->violationVarId());
  engine.endCommit();
  EXPECT_GT(engine.committedValue(node->violationVarId()), 0);
}
