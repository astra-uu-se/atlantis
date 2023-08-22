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
class AbstractArrayBoolAndNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::BoolVar> a;
  std::unique_ptr<fznparser::BoolVar> b;
  std::unique_ptr<fznparser::BoolVar> r;

  std::unique_ptr<invariantgraph::ArrayBoolAndNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = boolVar("a");
    b = boolVar("b");
    r = boolVar("r");

    fznparser::BoolVarArray inputs{""};
    inputs.append(*a);
    inputs.append(*b);

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "array_bool_and",
          std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{*r}})));

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
    node = makeNode<invariantgraph::ArrayBoolAndNode>(
        _model->constraints().front());
  }

  void construction() {
    EXPECT_EQ(node->staticInputVarNodeIds().size(), 2);
    EXPECT_EQ(node->dynamicInputVarNodeIds().size(), 0);
    std::vector<invariantgraph::VarNodeId> expectedVars{_nodeMap->at("a"),
                                                        _nodeMap->at("b")};
    EXPECT_EQ(node->staticInputVarNodeIds(), expectedVars);
    EXPECT_THAT(expectedVars,
                testing::ContainerEq(node->staticInputVarNodeIds()));
    expectMarkedAsInput(node.get(), node->staticInputVarNodeIds());
    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(node->isReified());
      EXPECT_EQ(node->reifiedViolation(), nullptr);
    } else {
      EXPECT_TRUE(node->isReified());
      EXPECT_NE(node->reifiedViolation(), nullptr);
      EXPECT_EQ(node->reifiedViolation()->variable(),
                invariantgraph::VarNode::FZNVariable(*r));
    }
  }

  void application() {
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
    addVariablesToEngine(engine);
    node->registerOutputVariables(engine);
    node->registerNode(*_invariantGraph, engine);

    std::vector<VarId> inputs;
    EXPECT_EQ(node->staticInputVarNodeIds().size(), 2);
    for (auto* const inputVariable : node->staticInputVarNodeIds()) {
      EXPECT_NE(inputVariable->varId(), NULL_ID);
      inputs.emplace_back(inputVariable->varId());
      engine.updateBounds(inputVariable->varId(), 0, 10, true);
    }

    EXPECT_NE(node->violationVarId(), NULL_ID);
    const VarId violationId = node->violationVarId();

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