#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"

class LinearNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> a;
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::IntVar> c;
  Int d{3};

  std::unique_ptr<invariantgraph::IntLinearNode> shouldRegisterView;
  std::unique_ptr<invariantgraph::IntLinearNode> shouldRegisterLinear;

  void makeView() {
    fznparser::IntVarArray coeffs("");
    coeffs.append(1);
    coeffs.append(1);

    fznparser::IntVarArray inputs("");
    inputs.append(*a);
    inputs.append(*b);

    makeNode<invariantgraph::IntLinearNode>(
        _model->addConstraint(fznparser::Constraint(
            "int_lin_eq",
            std::vector<fznparser::Arg>{coeffs, inputs, fznparser::IntArg{d}},
            std::vector<fznparser::Annotation>{
                definesVarAnnotation(a->identifier())})));
  }

  void makeLinear() {
    fznparser::IntVarArray coeffs("");
    coeffs.append(1);
    coeffs.append(1);
    coeffs.append(1);

    fznparser::IntVarArray inputs("");
    inputs.append(*a);
    inputs.append(*b);
    inputs.append(*c);

    shouldRegisterLinear = makeNode<invariantgraph::IntLinearNode>(
        _model->addConstraint(fznparser::Constraint(
            "int_lin_eq",
            std::vector<fznparser::Arg>{coeffs, inputs, fznparser::IntArg{d}},
            std::vector<fznparser::Annotation>{
                definesVarAnnotation(a->identifier())})));
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    a = intVar(0, 10, "a");
    b = intVar(3, 7, "b");
    c = intVar(3, 7, "c");
  }
};

TEST_F(LinearNodeTest, construction_should_register_view) {
  makeView();

  EXPECT_EQ(shouldRegisterView->coeffs().size(), 1);
  EXPECT_EQ(shouldRegisterView->coeffs()[0], 1);

  EXPECT_EQ(shouldRegisterView->staticInputVarNodeIds().size(), 1);
  EXPECT_EQ(shouldRegisterView->staticInputVarNodeIds()[0]->variable(),
            invariantgraph::VarNode::FZNVariable(*b));
  EXPECT_EQ(shouldRegisterView->staticInputVarNodeIds()[0]->inputFor().size(),
            1);
  EXPECT_EQ(shouldRegisterView->staticInputVarNodeIds()[0]->inputFor()[0],
            shouldRegisterView.get());

  EXPECT_EQ(shouldRegisterView->outputVarNodeIds().size(), 1);
  EXPECT_EQ(shouldRegisterView->outputVarNodeIds()[0]->variable(),
            invariantgraph::VarNode::FZNVariable(*a));
}

TEST_F(LinearNodeTest, construction_should_register_linear) {
  makeLinear();

  EXPECT_EQ(shouldRegisterLinear->coeffs().size(), 2);
  EXPECT_EQ(shouldRegisterLinear->coeffs()[0], 1);
  EXPECT_EQ(shouldRegisterLinear->coeffs()[1], 1);

  EXPECT_EQ(shouldRegisterLinear->staticInputVarNodeIds().size(), 2);
  EXPECT_EQ(shouldRegisterLinear->staticInputVarNodeIds()[0]->variable(),
            invariantgraph::VarNode::FZNVariable(*b));
  EXPECT_EQ(shouldRegisterLinear->staticInputVarNodeIds()[0]->inputFor().size(),
            1);
  EXPECT_EQ(shouldRegisterLinear->staticInputVarNodeIds()[0]->inputFor()[0],
            shouldRegisterLinear.get());

  EXPECT_EQ(shouldRegisterLinear->staticInputVarNodeIds()[1]->variable(),
            invariantgraph::VarNode::FZNVariable(*c));
  EXPECT_EQ(shouldRegisterLinear->staticInputVarNodeIds()[1]->inputFor().size(),
            1);
  EXPECT_EQ(shouldRegisterLinear->staticInputVarNodeIds()[1]->inputFor()[0],
            shouldRegisterLinear.get());

  EXPECT_EQ(shouldRegisterLinear->outputVarNodeIds().size(), 1);
  EXPECT_EQ(shouldRegisterLinear->outputVarNodeIds()[0]->variable(),
            invariantgraph::VarNode::FZNVariable(*a));
}

TEST_F(LinearNodeTest, application_should_register_view) {
  makeView();

  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  for (auto* const definedVariable : shouldRegisterView->outputVarNodeIds()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  shouldRegisterView->registerOutputVariables(engine);
  for (auto* const definedVariable : shouldRegisterView->outputVarNodeIds()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  shouldRegisterView->registerNode(engine);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(*a)), -4);
  EXPECT_EQ(engine.upperBound(engineVariable(*a)), 0);

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b
  EXPECT_EQ(engine.numVariables(), 1);
}

TEST_F(LinearNodeTest, application_should_register_linear) {
  makeLinear();
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  for (auto* const definedVariable : shouldRegisterLinear->outputVarNodeIds()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  shouldRegisterLinear->registerOutputVariables(engine);
  for (auto* const definedVariable : shouldRegisterLinear->outputVarNodeIds()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  shouldRegisterLinear->registerNode(engine);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(*a)), -11);
  EXPECT_EQ(engine.upperBound(engineVariable(*a)), -3);

  // b and c
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // b, c, intermediate (a is a view, and is not counted here)
  EXPECT_EQ(engine.numVariables(), 3);

  // linear
  EXPECT_EQ(engine.numInvariants(), 1);
}