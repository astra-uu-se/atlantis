#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElement2dNode.hpp"

class ArrayBoolElement2dNodeTest : public NodeTestBase {
 public:
  std::vector<std::vector<bool>> parMatrix{std::vector<bool>{true, false},
                                           std::vector<bool>{false, true}};

  std::unique_ptr<fznparser::IntVar> idx1;
  std::unique_ptr<fznparser::IntVar> idx2;
  std::unique_ptr<fznparser::BoolVar> y;

  std::unique_ptr<invariantgraph::ArrayIntElement2dNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    idx1 = intVar(1, 2, "idx1");
    idx2 = intVar(1, 2, "idx2");
    y = boolVar("y");

    fznparser::BoolVarArray argMatrix("");
    for (const auto& row : parMatrix) {
      for (const auto& elem : row) {
        argMatrix.append(elem);
      }
    }

    node = makeNode<invariantgraph::ArrayIntElement2dNode>(
        _model->addConstraint(fznparser::Constraint(
            "array_bool_element2d_nonshifted_flat",
            std::vector<fznparser::Arg>{
                fznparser::IntArg{*idx1}, fznparser::IntArg{*idx2}, argMatrix,
                fznparser::BoolArg{*y},
                fznparser::IntArg{static_cast<Int>(parMatrix.size())},
                fznparser::IntArg{1}, fznparser::IntArg{1}})));
  }
};

TEST_F(ArrayBoolElement2dNodeTest, construction) {
  EXPECT_EQ(*node->idx1()->variable(),
            invariantgraph::VarNode::FZNVariable(*idx1));
  EXPECT_EQ(*node->idx2()->variable(),
            invariantgraph::VarNode::FZNVariable(*idx2));

  EXPECT_EQ(node->outputVarNodeIds().size(), 1);
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*y));
  expectMarkedAsInput(node.get(), {node->dynamicInputVarNodeIds()});
  expectMarkedAsInput(node.get(), {node->idx1()});
  expectMarkedAsInput(node.get(), {node->idx2()});

  EXPECT_EQ(node->dynamicInputVarNodeIds().size(), 0);
}

TEST_F(ArrayBoolElement2dNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  for (auto* const definedVariable : node->outputVarNodeIds()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->registerOutputVariables(engine);
  for (auto* const definedVariable : node->outputVarNodeIds()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerNode(*_invariantGraph, engine);
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
  addVariablesToEngine(engine);
  node->registerOutputVariables(engine);
  node->registerNode(*_invariantGraph, engine);

  EXPECT_EQ(node->staticInputVarNodeIds().size(), 2);
  EXPECT_NE(node->staticInputVarNodeIds().front()->varId(), NULL_ID);

  EXPECT_EQ(node->dynamicInputVarNodeIds().size(), 0);

  EXPECT_NE(node->outputVarNodeIds().front()->varId(), NULL_ID);
  const VarId outputId = node->outputVarNodeIds().front()->varId();

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
