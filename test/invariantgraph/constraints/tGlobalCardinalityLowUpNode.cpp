#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

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
    : public NodeTestBase<GlobalCardinalityLowUpNode> {
 public:
  VarNodeId x1;
  VarNodeId x2;
  const std::vector<Int> cover{2, 6};
  const std::vector<Int> low{0, 1};
  const std::vector<Int> up{1, 2};
  VarNodeId r;

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
    std::vector<VarNodeId> expectedInputs{x1, x2};
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    if (Type == ConstraintType::REIFIED) {
      EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
      EXPECT_EQ(invNode().outputVarNodeIds().front(), r);
    } else {
      EXPECT_EQ(invNode().outputVarNodeIds().size(), 0);
    }

    if constexpr (Type == ConstraintType::REIFIED) {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    } else {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    }
  }

  void application() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);

    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerOutputVariables(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);

    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    // x1, x2
    EXPECT_EQ(solver.searchVariables().size(), 2);
    // x1, x2, violation
    // violation
    EXPECT_EQ(solver.numVariables(), 3);
    // gcc
    EXPECT_EQ(solver.numInvariants(), 1);
    EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVariables(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputs;
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }
    EXPECT_EQ(inputs.size(), 2);

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);

    std::vector<Int> inputVals(inputs.size());

    solver.close();

    for (inputVals.at(0) = solver.lowerBound(inputs.at(0));
         inputVals.at(0) <= solver.upperBound(inputs.at(0));
         ++inputVals.at(0)) {
      for (inputVals.at(1) = solver.lowerBound(inputs.at(1));
           inputVals.at(1) <= solver.upperBound(inputs.at(1));
           ++inputVals.at(1)) {
        solver.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          solver.setValue(inputs.at(i), inputVals.at(i));
        }
        solver.endMove();

        solver.beginProbe();
        solver.query(violationId);
        solver.endProbe();

        if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
          bool sat = isSatisfied(inputVals, cover, low, up);
          EXPECT_EQ(solver.currentValue(violationId) == 0, sat);
        } else {
          bool sat = isSatisfied(inputVals, cover, low, up);
          EXPECT_NE(solver.currentValue(violationId) == 0, sat);
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

}  // namespace atlantis::testing