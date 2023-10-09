#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  for (const Int val : values) {
    if (val > 0) {
      return true;
    }
  }
  return false;
}

template <ConstraintType Type>
class AbstractArrayBoolAndNodeTest
    : public NodeTestBase<invariantgraph::ArrayBoolAndNode> {
 public:
  invariantgraph::VarNodeId a;
  invariantgraph::VarNodeId b;
  invariantgraph::VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createBoolVar("b");
    r = createBoolVar("r");

    fznparser::BoolVarArray inputs{""};
    inputs.append(boolVar(a));
    inputs.append(boolVar(b));

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "array_bool_and", std::vector<fznparser::Arg>{
                                inputs, fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_and",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_and",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_and",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
    std::vector<invariantgraph::VarNodeId> expectedVars{a, b};
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedVars);
    EXPECT_THAT(expectedVars,
                testing::ContainerEq(invNode().staticInputVarNodeIds()));
    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(),
                invariantgraph::NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(),
                invariantgraph::NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    }
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), NULL_ID);
    }
    invNode().registerOutputVariables(*_invariantGraph, engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), NULL_ID);
    }
    invNode().registerNode(*_invariantGraph, engine);
    engine.close();

    // a and b
    EXPECT_EQ(engine.searchVariables().size(), 2);

    // a, b and r
    EXPECT_EQ(engine.numVariables(), 3);

    // sum
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
      engine.updateBounds(varId(inputVarNodeId), 0, 10, true);
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), NULL_ID);
    const VarId violationId = invNode().violationVarId(*_invariantGraph);

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

class ArrayBoolAndNodeTest
    : public AbstractArrayBoolAndNodeTest<ConstraintType::NORMAL> {};

TEST_F(ArrayBoolAndNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndNodeTest, Propagation) { propagation(); }

class ArrayBoolAndReifNodeTest
    : public AbstractArrayBoolAndNodeTest<ConstraintType::REIFIED> {};

TEST_F(ArrayBoolAndReifNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndReifNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndReifNodeTest, Propagation) { propagation(); }

class ArrayBoolAndFalseNodeTest
    : public AbstractArrayBoolAndNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(ArrayBoolAndFalseNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndFalseNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndFalseNodeTest, Propagation) { propagation(); }

class ArrayBoolAndTrueNodeTest
    : public AbstractArrayBoolAndNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(ArrayBoolAndTrueNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolAndTrueNodeTest, Application) { application(); }

TEST_F(ArrayBoolAndTrueNodeTest, Propagation) { propagation(); }