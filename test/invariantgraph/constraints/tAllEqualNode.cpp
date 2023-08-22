#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/allEqualNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if (values.at(i) != values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

template <ConstraintType Type>
class AbstractAllEqualNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> a;
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::IntVar> c;
  std::unique_ptr<fznparser::IntVar> d;
  std::unique_ptr<fznparser::BoolVar> r;

  std::unique_ptr<invariantgraph::AllEqualNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = intVar(5, 10, "a");
    b = intVar(2, 7, "b");
    c = intVar(2, 7, "c");
    d = intVar(2, 7, "d");
    r = boolVar("r");

    fznparser::IntVarArray inputs{""};
    inputs.append(*a);
    inputs.append(*b);
    inputs.append(*c);
    inputs.append(*d);
    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "fzn_all_equal_int_reif",
          std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{*r}})));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_all_equal_int", std::vector<fznparser::Arg>{inputs})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_all_equal_int_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_all_equal_int_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      }
    }
    node =
        makeNode<invariantgraph::AllEqualNode>(_model->constraints().front());
  }

  void construction() {
    EXPECT_EQ(node->staticInputVarNodeIds().size(), 4);
    std::vector<invariantgraph::VarNodeId> expectedVars{
        _nodeMap->at("a"), _nodeMap->at("b"), _nodeMap->at("c"),
        _nodeMap->at("d")};
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
    EXPECT_EQ(node->violationVarId(), NULL_ID);
    node->registerOutputVariables(engine);
    for (auto* const definedVariable : node->outputVarNodeIds()) {
      EXPECT_NE(definedVariable->varId(), NULL_ID);
    }
    EXPECT_NE(node->violationVarId(), NULL_ID);
    node->registerNode(*_invariantGraph, engine);
    engine.close();

    // a, b, c and d
    EXPECT_EQ(engine.searchVariables().size(), 4);

    // a, b, c, d and the violation
    EXPECT_EQ(engine.numVariables(), 5);

    // alldifferent
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(node->violationVarId()), 0);
    EXPECT_GT(engine.upperBound(node->violationVarId()), 0);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    addVariablesToEngine(engine);
    node->registerOutputVariables(engine);
    node->registerNode(*_invariantGraph, engine);

    std::vector<VarId> inputs;
    EXPECT_EQ(node->staticInputVarNodeIds().size(), 4);
    for (auto* const inputVariable : node->staticInputVarNodeIds()) {
      EXPECT_NE(inputVariable->varId(), NULL_ID);
      inputs.emplace_back(inputVariable->varId());
    }

    EXPECT_NE(node->violationVarId(), NULL_ID);
    const VarId violationId = node->violationVarId();
    EXPECT_EQ(inputs.size(), 4);

    std::vector<Int> values(inputs.size());
    engine.close();

    for (values.at(0) = engine.lowerBound(inputs.at(0));
         values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = engine.lowerBound(inputs.at(1));
           values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = engine.lowerBound(inputs.at(2));
             values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
          for (values.at(3) = engine.lowerBound(inputs.at(3));
               values.at(3) <= engine.upperBound(inputs.at(3));
               ++values.at(3)) {
            engine.beginMove();
            for (size_t i = 0; i < inputs.size(); ++i) {
              engine.setValue(inputs.at(i), values.at(i));
            }
            engine.endMove();

            engine.beginProbe();
            engine.query(violationId);
            engine.endProbe();

            if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
              EXPECT_EQ(engine.currentValue(violationId) > 0,
                        isViolating(values));
            } else {
              EXPECT_NE(engine.currentValue(violationId) > 0,
                        isViolating(values));
            }
          }
        }
      }
    }
  }
};

class AllEqualNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::NORMAL> {};

TEST_F(AllEqualNodeTest, Construction) { construction(); }

TEST_F(AllEqualNodeTest, Application) { application(); }

TEST_F(AllEqualNodeTest, Propagation) { propagation(); }

class AllEqualReifNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::REIFIED> {};

TEST_F(AllEqualReifNodeTest, Construction) { construction(); }

TEST_F(AllEqualReifNodeTest, Application) { application(); }

TEST_F(AllEqualReifNodeTest, Propagation) { propagation(); }

class AllEqualFalseNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(AllEqualFalseNodeTest, Construction) { construction(); }

TEST_F(AllEqualFalseNodeTest, Application) { application(); }

TEST_F(AllEqualFalseNodeTest, Propagation) { propagation(); }

class AllEqualTrueNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(AllEqualTrueNodeTest, Construction) { construction(); }

TEST_F(AllEqualTrueNodeTest, Application) { application(); }

TEST_F(AllEqualTrueNodeTest, Propagation) { propagation(); }