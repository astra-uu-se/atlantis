#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/boolLeNode.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& values) {
  return (values.front() == 0) > (values.back() == 0);
}

template <ConstraintType Type>
class AbstractBoolLeNodeTest : public NodeTestBase<BoolLeNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createBoolVar("b");
    r = createBoolVar("r");

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "bool_le_reif",
          std::vector<fznparser::Arg>{fznparser::BoolArg{boolVar(a)},
                                      fznparser::BoolArg{boolVar(b)},
                                      fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_le",
            std::vector<fznparser::Arg>{fznparser::BoolArg{boolVar(a)},
                                        fznparser::BoolArg{boolVar(b)}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_le_reif",
            std::vector<fznparser::Arg>{fznparser::BoolArg{boolVar(a)},
                                        fznparser::BoolArg{boolVar(b)},
                                        fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_le_reif",
            std::vector<fznparser::Arg>{fznparser::BoolArg{boolVar(a)},
                                        fznparser::BoolArg{boolVar(b)},
                                        fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().a(), a);
    EXPECT_EQ(invNode().b(), b);
    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    }
  }

  void application() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerNode(*_invariantGraph, engine);
    engine.close();

    // a and b
    EXPECT_EQ(engine.searchVariables().size(), 2);

    // a, b and the violation
    EXPECT_EQ(engine.numVariables(), 3);

    // equal
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_EQ(engine.upperBound(invNode().violationVarId(*_invariantGraph)), 1);
  }

  void propagation() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    invNode().registerNode(*_invariantGraph, engine);

    std::vector<propagation::VarId> inputs;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
      engine.updateBounds(varId(inputVarNodeId), 0, 5, true);
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId = invNode().violationVarId(*_invariantGraph);
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

        if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
          EXPECT_EQ(engine.currentValue(violationId) > 0, isViolating(values));
        } else {
          EXPECT_NE(engine.currentValue(violationId) > 0, isViolating(values));
        }
      }
    }
  }
};

class BoolLeNodeTest : public AbstractBoolLeNodeTest<ConstraintType::NORMAL> {};

TEST_F(BoolLeNodeTest, Construction) { construction(); }

TEST_F(BoolLeNodeTest, Application) { application(); }

TEST_F(BoolLeNodeTest, Propagation) { propagation(); }

class BoolLeReifNodeTest
    : public AbstractBoolLeNodeTest<ConstraintType::REIFIED> {};

TEST_F(BoolLeReifNodeTest, Construction) { construction(); }

TEST_F(BoolLeReifNodeTest, Application) { application(); }

TEST_F(BoolLeReifNodeTest, Propagation) { propagation(); }

class BoolLeFalseNodeTest
    : public AbstractBoolLeNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(BoolLeFalseNodeTest, Construction) { construction(); }

TEST_F(BoolLeFalseNodeTest, Application) { application(); }

TEST_F(BoolLeFalseNodeTest, Propagation) { propagation(); }

class BoolLeTrueNodeTest
    : public AbstractBoolLeNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(BoolLeTrueNodeTest, Construction) { construction(); }

TEST_F(BoolLeTrueNodeTest, Application) { application(); }

TEST_F(BoolLeTrueNodeTest, Propagation) { propagation(); }
}