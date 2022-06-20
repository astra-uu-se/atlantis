#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayBoolElement2dNode.hpp"
#include "invariantgraph/invariants/arrayIntElement2dNode.hpp"

class ArrayBoolElement2dNodeTest : public NodeTestBase {
 public:
  std::vector<std::vector<bool>> parMatrix{std::vector<bool>{true, false},
                                           std::vector<bool>{false, true}};

  INT_VARIABLE(idx1, 1, 2);
  INT_VARIABLE(idx2, 1, 2);
  BOOL_VARIABLE(y);

  fznparser::Constraint constraint{
      "array_bool_element2d_nonshifted_flat",
      {"idx1", "idx2",
       fznparser::Constraint::ArrayArgument{
           parMatrix.at(0).at(0), parMatrix.at(0).at(1), parMatrix.at(1).at(0),
           parMatrix.at(1).at(1)},
       "y", static_cast<Int>(parMatrix.size()), 1, 1},
      {}};

  fznparser::FZNModel model{
      {}, {idx1, idx2, y}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayIntElement2dNode> node;

  void SetUp() override {
    setModel(&model);
    node = invariantgraph::ArrayBoolElement2dNode::fromModelConstraint(
        *_model, constraint, nodeFactory);
  }
};

TEST_F(ArrayBoolElement2dNodeTest, construction) {
  EXPECT_EQ(*node->idx1()->variable(),
            invariantgraph::VariableNode::FZNVariable(idx1));
  EXPECT_EQ(*node->idx2()->variable(),
            invariantgraph::VariableNode::FZNVariable(idx2));

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
            invariantgraph::VariableNode::FZNVariable(y));
  expectMarkedAsInput(node.get(), {node->dynamicInputs()});
  expectMarkedAsInput(node.get(), {node->idx1()});
  expectMarkedAsInput(node.get(), {node->idx2()});

  EXPECT_EQ(node->dynamicInputs().size(), 0);
}

TEST_F(ArrayBoolElement2dNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {idx1.name, idx2.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->createDefinedVariables(engine);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerWithEngine(engine);
  engine.close();

  // idx1, idx2
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // idx1, idx2, and y
  EXPECT_EQ(engine.numVariables(), 3);

  // element2dVar
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayBoolElement2dNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {idx1.name, idx2.name});
  node->createDefinedVariables(engine);
  node->registerWithEngine(engine);

  EXPECT_EQ(node->staticInputs().size(), 2);
  EXPECT_NE(node->staticInputs().front()->varId(), NULL_ID);

  EXPECT_EQ(node->dynamicInputs().size(), 0);

  EXPECT_NE(node->definedVariables().front()->varId(), NULL_ID);
  const VarId outputId = node->definedVariables().front()->varId();

  std::vector<VarId> inputs;
  inputs.emplace_back(node->idx1()->varId());
  inputs.emplace_back(node->idx2()->varId());
  engine.close();
  std::vector<Int> values(inputs.size(), 0);

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

      const Int actual = engine.currentValue(outputId);
      const Int row = values.at(0) - 1;  // offset of 1
      const Int col = values.at(1) - 1;  // offset of 1

      EXPECT_EQ(actual == 0, parMatrix.at(row).at(col));
    }
  }
}
