#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/boolLinLeNode.hpp"

static bool isViolating(const std::vector<Int>& coeffs,
                        const std::vector<Int>& values, const Int expected) {
  Int sum = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    sum += coeffs.at(i) * static_cast<Int>(values.at(i) == 0);
  }
  return sum > expected;
}

class BoolLinLeNodeTest : public NodeTestBase<invariantgraph::BoolLinLeNode> {
 public:
  invariantgraph::VarNodeId a;
  invariantgraph::VarNodeId b;
  invariantgraph::VarNodeId r;

  Int sum{3};
  std::vector<Int> coeffs{1, 2};

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createBoolVar("b");
    r = createBoolVar("r");

    fznparser::IntVarArray coeffsArg("");
    for (int64_t c : coeffs) {
      coeffsArg.append(c);
    }
    fznparser::BoolVarArray varsArg("");
    varsArg.append(boolVar(a));
    varsArg.append(boolVar(b));

    _model->addConstraint(std::move(fznparser::Constraint(
        "bool_lin_le", std::vector<fznparser::Arg>{coeffsArg, varsArg,
                                                   fznparser::IntArg{sum}})));

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().at(0), a);
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(1), b);
    expectInputTo(invNode());

    EXPECT_EQ(invNode().coeffs().at(0), 1);
    EXPECT_EQ(invNode().coeffs().at(1), 2);
    EXPECT_EQ(invNode().bound(), 3);
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), invariantgraph::NULL_NODE_ID);
    EXPECT_EQ(
        _invariantGraph->varNode(invNode().reifiedViolationNodeId()).variable(),
        invariantgraph::VarNode::FZNVariable(boolVar(r)));
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), NULL_ID);
    }
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), NULL_ID);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), NULL_ID);
    invNode().registerNode(*_invariantGraph, engine);
    engine.close();

    // a and b
    EXPECT_EQ(engine.searchVariables().size(), 2);

    // a, b, the linear sum of a and b
    EXPECT_EQ(engine.numVariables(), 3);

    // linear
    EXPECT_EQ(engine.numInvariants(), 1);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    invNode().registerNode(*_invariantGraph, engine);

    std::vector<VarId> inputs;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
      engine.updateBounds(varId(inputVarNodeId), 0, 3, true);
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), NULL_ID);
    const VarId violationId = invNode().violationVarId(*_invariantGraph);
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

        EXPECT_EQ(viol != 0, isViolating(coeffs, values, sum));
      }
    }
  }
};

TEST_F(BoolLinLeNodeTest, Construction) {}

TEST_F(BoolLinLeNodeTest, Application) {}

TEST_F(BoolLinLeNodeTest, Propagation) {}
