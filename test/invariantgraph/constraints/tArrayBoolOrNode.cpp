#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  for (const Int val : values) {
    if (val == 0) {
      return false;
    }
  }
  return true;
}

template <ConstraintType Type>
class AbstractArrayBoolOrNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::BoolVar> a;
  std::unique_ptr<fznparser::BoolVar> b;
  std::unique_ptr<fznparser::BoolVar> r;

  std::unique_ptr<invariantgraph::ArrayBoolOrNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = boolVar("a");
    b = boolVar("b");
    r = boolVar("r");

    fznparser::BoolVarArray inputs("");
    inputs.append(*a);
    inputs.append(*b);

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "array_bool_or",
          std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{*r}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_or",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_or",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_or",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      }
    }

    node = makeNode<invariantgraph::ArrayBoolOrNode>(
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

    // minSparse
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

class ArrayBoolOrNodeTest
    : public AbstractArrayBoolOrNodeTest<ConstraintType::NORMAL> {};

TEST_F(ArrayBoolOrNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrNodeTest, Propagation) { propagation(); }

class ArrayBoolOrReifNodeTest
    : public AbstractArrayBoolOrNodeTest<ConstraintType::REIFIED> {};

TEST_F(ArrayBoolOrReifNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrReifNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrReifNodeTest, Propagation) { propagation(); }

class ArrayBoolOrFalseNodeTest
    : public AbstractArrayBoolOrNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(ArrayBoolOrFalseNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrFalseNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrFalseNodeTest, Propagation) { propagation(); }

class ArrayBoolOrTrueNodeTest
    : public AbstractArrayBoolOrNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(ArrayBoolOrTrueNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrTrueNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrTrueNodeTest, Propagation) { propagation(); }