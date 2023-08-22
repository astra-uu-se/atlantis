#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

static bool isSatisfied(const std::vector<Int>& values,
                        const std::vector<Int>& cover,
                        const std::vector<Int>& low,
                        const std::vector<Int>& up) {
  std::vector<Int> counts(cover.size(), 0);
  for (const Int val : values) {
    for (size_t i = 0; i < cover.size(); ++i) {
      if (val == cover.at(i)) {
        counts.at(i)++;
        break;
      }
    }
  }
  bool sat = true;
  for (size_t i = 0; i < counts.size(); ++i) {
    sat = sat && low.at(i) <= counts.at(i) && counts.at(i) <= up.at(i);
  }
  return sat;
}

template <ConstraintType Type>
class AbstractGlobalCardinalityLowUpNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> x1;
  std::unique_ptr<fznparser::IntVar> x2;
  const std::vector<Int> cover{2, 6};
  const std::vector<Int> low{0, 1};
  const std::vector<Int> up{1, 2};
  std::unique_ptr<fznparser::BoolVar> r;

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;
  std::unique_ptr<invariantgraph::GlobalCardinalityLowUpNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = intVar(5, 10, "x1");
    x2 = intVar(2, 7, "x2");
    r = boolVar("r");

    fznparser::IntVarArray inputArg("");
    inputArg.append(*x1);
    inputArg.append(*x2);

    fznparser::IntVarArray coverArg("");
    for (const Int val : cover) {
      coverArg.append(val);
    }

    fznparser::IntVarArray lowArg("");
    for (const Int val : low) {
      lowArg.append(val);
    }

    fznparser::IntVarArray upArg("");
    for (const Int val : up) {
      upArg.append(val);
    }

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "fzn_global_cardinality_low_up_reif",
          std::vector<fznparser::Arg>{inputArg, coverArg, lowArg, upArg,
                                      fznparser::BoolArg{*r}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_low_up_closed",
            std::vector<fznparser::Arg>{inputArg, coverArg, lowArg, upArg})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_low_up_reif",
            std::vector<fznparser::Arg>{inputArg, coverArg, lowArg, upArg,
                                        fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_low_up_reif",
            std::vector<fznparser::Arg>{inputArg, coverArg, lowArg, upArg,
                                        fznparser::BoolArg{true}})));
      }
    }

    node = makeNode<invariantgraph::GlobalCardinalityLowUpNode>(
        _model->constraints().front());
  }

  void construction() {
    const size_t numInputs = 2;
    EXPECT_EQ(node->staticInputVarNodeIds().size(), numInputs);
    std::vector<invariantgraph::VarNodeId> expectedInputs{_nodeMap->at("x1"),
                                                          _nodeMap->at("x2")};
    EXPECT_EQ(node->staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs,
                testing::ContainerEq(node->staticInputVarNodeIds()));
    expectMarkedAsInput(node.get(), node->staticInputVarNodeIds());

    if (Type == ConstraintType::REIFIED) {
      EXPECT_EQ(node->outputVarNodeIds().size(), 1);
      EXPECT_EQ(node->outputVarNodeIds().front(), _nodeMap->at("r"));
    } else {
      EXPECT_EQ(node->outputVarNodeIds().size(), 0);
    }

    if constexpr (Type == ConstraintType::REIFIED) {
      EXPECT_TRUE(node->isReified());
      EXPECT_NE(node->reifiedViolation(), nullptr);
      EXPECT_EQ(node->reifiedViolation()->variable(),
                invariantgraph::VarNode::FZNVariable(*r));
    } else {
      EXPECT_FALSE(node->isReified());
      EXPECT_EQ(node->reifiedViolation(), nullptr);
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

    // x1, x2
    EXPECT_EQ(engine.searchVariables().size(), 2);
    // x1, x2, violation
    // violation
    EXPECT_EQ(engine.numVariables(), 3);
    // gcc
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
    for (auto* const input : node->staticInputVarNodeIds()) {
      EXPECT_NE(input->varId(), NULL_ID);
      inputs.emplace_back(input->varId());
    }
    EXPECT_EQ(inputs.size(), 2);

    EXPECT_NE(node->violationVarId(), NULL_ID);
    const VarId violationId = node->violationVarId();

    std::vector<Int> inputVals(inputs.size());

    engine.close();

    for (inputVals.at(0) = engine.lowerBound(inputs.at(0));
         inputVals.at(0) <= engine.upperBound(inputs.at(0));
         ++inputVals.at(0)) {
      for (inputVals.at(1) = engine.lowerBound(inputs.at(1));
           inputVals.at(1) <= engine.upperBound(inputs.at(1));
           ++inputVals.at(1)) {
        engine.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          engine.setValue(inputs.at(i), inputVals.at(i));
        }
        engine.endMove();

        engine.beginProbe();
        engine.query(violationId);
        engine.endProbe();

        if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
          bool sat = isSatisfied(inputVals, cover, low, up);
          EXPECT_EQ(engine.currentValue(violationId) == 0, sat);
        } else {
          bool sat = isSatisfied(inputVals, cover, low, up);
          EXPECT_NE(engine.currentValue(violationId) == 0, sat);
        }
      }
    }
  }
};

class GlobalCardinalityLowUpNodeTest
    : public AbstractGlobalCardinalityLowUpNodeTest<ConstraintType::NORMAL> {};

TEST_F(GlobalCardinalityLowUpNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityLowUpNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityLowUpNodeTest, Propagation) { propagation(); }

class GlobalCardinalityLowUpReifNodeTest
    : public AbstractGlobalCardinalityLowUpNodeTest<ConstraintType::REIFIED> {};

TEST_F(GlobalCardinalityLowUpReifNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityLowUpReifNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityLowUpReifNodeTest, Propagation) { propagation(); }

class GlobalCardinalityLowUpFalseNodeTest
    : public AbstractGlobalCardinalityLowUpNodeTest<
          ConstraintType::CONSTANT_FALSE> {};

TEST_F(GlobalCardinalityLowUpFalseNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityLowUpFalseNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityLowUpFalseNodeTest, Propagation) { propagation(); }

class GlobalCardinalityLowUpTrueNodeTest
    : public AbstractGlobalCardinalityLowUpNodeTest<
          ConstraintType::CONSTANT_TRUE> {};

TEST_F(GlobalCardinalityLowUpTrueNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityLowUpTrueNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityLowUpTrueNodeTest, Propagation) { propagation(); }