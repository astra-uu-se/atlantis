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
class AbstractGlobalCardinalityLowUpNodeTest
    : public NodeTestBase<invariantgraph::GlobalCardinalityLowUpNode> {
 public:
  invariantgraph::VarNodeId x1;
  invariantgraph::VarNodeId x2;
  const std::vector<Int> cover{2, 6};
  const std::vector<Int> low{0, 1};
  const std::vector<Int> up{1, 2};
  invariantgraph::VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVar(5, 10, "x1");
    x2 = createIntVar(2, 7, "x2");
    r = createBoolVar("r");

    fznparser::IntVarArray inputArg("");
    inputArg.append(intVar(x1));
    inputArg.append(intVar(x2));

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
                                      fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_low_up",
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

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    const size_t numInputs = 2;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numInputs);
    std::vector<invariantgraph::VarNodeId> expectedInputs{x1, x2};
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs,
                testing::ContainerEq(invNode().staticInputVarNodeIds()));

    if (Type == ConstraintType::REIFIED) {
      EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
      EXPECT_EQ(invNode().outputVarNodeIds().front(), r);
    } else {
      EXPECT_EQ(invNode().outputVarNodeIds().size(), 0);
    }

    if constexpr (Type == ConstraintType::REIFIED) {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(),
                invariantgraph::NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    } else {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(),
                invariantgraph::NULL_NODE_ID);
    }
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

    // x1, x2
    EXPECT_EQ(engine.searchVariables().size(), 2);
    // x1, x2, violation
    // violation
    EXPECT_EQ(engine.numVariables(), 3);
    // gcc
    EXPECT_EQ(engine.numInvariants(), 1);
    EXPECT_EQ(engine.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_GT(engine.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    invNode().registerNode(*_invariantGraph, engine);

    std::vector<VarId> inputs;
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }
    EXPECT_EQ(inputs.size(), 2);

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), NULL_ID);
    const VarId violationId = invNode().violationVarId(*_invariantGraph);

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