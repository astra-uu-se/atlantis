#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

class CircuitImplicitNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> a;
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::IntVar> c;
  std::unique_ptr<fznparser::IntVar> d;

  std::unique_ptr<invariantgraph::CircuitImplicitNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = intVar(1, 4, "a");
    b = intVar(1, 4, "b");
    c = intVar(1, 4, "c");
    d = intVar(1, 4, "d");

    fznparser::IntVarArray inputs("");
    inputs.append(*a);
    inputs.append(*b);
    inputs.append(*c);
    inputs.append(*d);

    node = makeNode<invariantgraph::CircuitImplicitNode>(
        _model->addConstraint(fznparser::Constraint(
            "fzn_circuit", std::vector<fznparser::Arg>{inputs})));
  }
};

TEST_F(CircuitImplicitNodeTest, construction) {
  std::vector<invariantgraph::VarNodeId> expectedVars{
      _nodeMap->at("a"), _nodeMap->at("b"), _nodeMap->at("c"),
      _nodeMap->at("d")};

  EXPECT_EQ(node->outputVarNodeIds(), expectedVars);
}

TEST_F(CircuitImplicitNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  for (auto* const definedVariable : node->outputVarNodeIds()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->registerOutputVariables(engine);
  for (auto* const definedVariable : node->outputVarNodeIds()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerNode(*_invariantGraph, engine);
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c and d
  EXPECT_EQ(engine.numVariables(), 4);

  EXPECT_EQ(engine.numInvariants(), 0);

  auto neighbourhood = node->takeNeighbourhood();
  EXPECT_FALSE(node->takeNeighbourhood());

  EXPECT_TRUE(dynamic_cast<search::neighbourhoods::CircuitNeighbourhood*>(
      neighbourhood.get()));
}
