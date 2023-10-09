#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"
#include "search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

class AllDifferentImplicitNodeTest
    : public NodeTestBase<invariantgraph::AllDifferentImplicitNode> {
 public:
  invariantgraph::VarNodeId a;
  invariantgraph::VarNodeId b;
  invariantgraph::VarNodeId c;
  invariantgraph::VarNodeId d;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(2, 7, "a");
    b = createIntVar(2, 7, "b");
    c = createIntVar(2, 7, "c");
    d = createIntVar(2, 7, "d");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(a));
    inputs.append(intVar(b));
    inputs.append(intVar(c));
    inputs.append(intVar(d));

    _model->addConstraint(fznparser::Constraint(
        "fzn_all_different_int", std::vector<fznparser::Arg>{inputs}));

    makeImplicitNode(_model->constraints().front());
  }
};

TEST_F(AllDifferentImplicitNodeTest, construction) {
  std::vector<invariantgraph::VarNodeId> expectedVars{a, b, c, d};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedVars);
}

TEST_F(AllDifferentImplicitNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), NULL_ID);
  }
  invNode().registerOutputVariables(*_invariantGraph, engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, engine);
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c and d
  EXPECT_EQ(engine.numVariables(), 4);

  EXPECT_EQ(engine.numInvariants(), 0);

  auto neighbourhood = invNode().neighbourhood();

  EXPECT_TRUE(
      dynamic_cast<search::neighbourhoods::AllDifferentUniformNeighbourhood*>(
          neighbourhood.get()));
}
