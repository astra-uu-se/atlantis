#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static std::pair<bool, std::vector<Int>> compute(
    const std::vector<Int>& values, const std::vector<Int>& cover,
    const std::vector<Int>& counts = {}) {
  bool sat = true;
  std::vector<Int> outputs(cover.size(), 0);
  for (const Int val : values) {
    bool inCover = false;
    for (size_t i = 0; i < cover.size(); ++i) {
      if (val == cover.at(i)) {
        outputs.at(i)++;
        inCover = true;
        break;
      }
    }
    sat = sat && inCover;
  }
  for (size_t i = 0; i < counts.size(); ++i) {
    sat = sat && outputs.at(i) == counts.at(i);
  }
  return std::pair<bool, std::vector<Int>>{sat, outputs};
}

template <ViolationInvariantType Type>
class AbstractGlobalCardinalityClosedNodeTest
    : public NodeTestBase<GlobalCardinalityClosedNode> {
 public:
  VarNodeId x1;
  VarNodeId x2;
  const std::vector<Int> cover{2, 6};
  VarNodeId o1;
  VarNodeId o2;
  VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVar(5, 10, "x1");
    x2 = createIntVar(2, 7, "x2");
    o1 = createIntVar(1, 2, "o1");
    o2 = createIntVar(1, 2, "o2");
    addFznVar("r");

    fznparser::IntVarArray inputsArg("");
    inputsArg.append(intVar(x1));
    inputsArg.append(intVar(x2));

    fznparser::IntVarArray coversArg("");
    for (const Int val : cover) {
      coversArg.append(val);
    }

    fznparser::IntVarArray outputsArg("");
    outputsArg.append(intVar(o1));
    outputsArg.append(intVar(o2));

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "fzn_global_cardinality_closed_reif",
          std::vector<fznparser::Arg>{inputsArg, coversArg, outputsArg,
                                      fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ViolationInvariantType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_closed",
            std::vector<fznparser::Arg>{inputsArg, coversArg, outputsArg})));
      } else if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_closed_reif",
            std::vector<fznparser::Arg>{inputsArg, coversArg, outputsArg,
                                        fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_closed_reif",
            std::vector<fznparser::Arg>{inputsArg, coversArg, outputsArg,
                                        fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    std::vector<VarNodeId> expectedInputs{x1, x2};
    std::vector<VarNodeId> expectedOutputs{o1, o2};
    if constexpr (Type == ViolationInvariantType::REIFIED ||
                  Type == ViolationInvariantType::CONSTANT_FALSE) {
      expectedInputs.emplace_back(o1);
      expectedInputs.emplace_back(o2);
      expectedOutputs.clear();
      if constexpr (Type == ViolationInvariantType::REIFIED) {
        expectedOutputs.emplace_back(r);
      }
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);

    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    EXPECT_EQ(invNode().outputVarNodeIds().size(), expectedOutputs.size());
    EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
    EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));

    if constexpr (Type == ViolationInvariantType::REIFIED) {
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
    invNode().registerOutputVars(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);

    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    if constexpr (Type == ViolationInvariantType::NORMAL ||
                  Type == ViolationInvariantType::CONSTANT_TRUE) {
      // x1, x2
      EXPECT_EQ(solver.searchVars().size(), 2);
      // x1, x2, o1, o2
      // violation
      EXPECT_EQ(solver.numVars(), 5);
      // gcc
      EXPECT_EQ(solver.numInvariants(), 1);
    } else {
      // x1, x2, o1, o2
      EXPECT_EQ(solver.searchVars().size(), 4);
      // x1, x2, o1, o2
      // intermediate o1, intermediate o2
      // 3 intermediate violations
      // 1 total violation
      EXPECT_EQ(solver.numVars(), 10);
      // gcc + 2 (non)-equal + 1 total violation
      EXPECT_EQ(solver.numInvariants(), 4);

      EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)),
                0);
      EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)),
                0);
    }
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVars(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputs;
    for (const auto& inputVarNodeId : invNode().inputs()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }
    EXPECT_EQ(inputs.size(), 2);

    std::vector<propagation::VarId> counts;
    for (const auto& countVarNodeId : invNode().counts()) {
      EXPECT_NE(varId(countVarNodeId), propagation::NULL_ID);
      counts.emplace_back(varId(countVarNodeId));
    }
    EXPECT_EQ(counts.size(), 2);

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);

    std::vector<Int> inputVals(inputs.size());
    std::vector<Int> countVals(counts.size());

    std::vector<std::pair<Int, Int>> countBounds;

    if constexpr (Type == ViolationInvariantType::NORMAL ||
                  Type == ViolationInvariantType::CONSTANT_TRUE) {
      for (const propagation::VarId c : counts) {
        countBounds.emplace_back(std::pair<Int, Int>{0, 0});
      }
    } else {
      for (const propagation::VarId c : counts) {
        countBounds.emplace_back(
            std::pair<Int, Int>{solver.lowerBound(c), solver.lowerBound(c)});
      }
    }

    solver.close();

    for (inputVals.at(0) = solver.lowerBound(inputs.at(0));
         inputVals.at(0) <= solver.upperBound(inputs.at(0));
         ++inputVals.at(0)) {
      for (inputVals.at(1) = solver.lowerBound(inputs.at(1));
           inputVals.at(1) <= solver.upperBound(inputs.at(1));
           ++inputVals.at(1)) {
        for (countVals.at(0) = countBounds.at(0).first;
             countVals.at(0) <= countBounds.at(0).second; ++countVals.at(0)) {
          for (countVals.at(1) = countBounds.at(1).first;
               countVals.at(1) <= countBounds.at(1).second; ++countVals.at(1)) {
            solver.beginMove();
            for (size_t i = 0; i < inputs.size(); ++i) {
              solver.setValue(inputs.at(i), inputVals.at(i));
            }
            if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE ||
                          Type == ViolationInvariantType::REIFIED) {
              for (size_t i = 0; i < counts.size(); ++i) {
                solver.setValue(counts.at(i), countVals.at(i));
              }
            }
            solver.endMove();

            solver.beginProbe();
            solver.query(violationId);
            solver.endProbe();

            if constexpr (Type == ViolationInvariantType::NORMAL ||
                          Type == ViolationInvariantType::CONSTANT_TRUE) {
              const auto& [sat, expectedCounts] = compute(inputVals, cover);
              EXPECT_EQ(solver.currentValue(violationId) == 0, sat);
              EXPECT_EQ(counts.size(), expectedCounts.size());
              for (size_t i = 0; i < countVals.size(); ++i) {
                EXPECT_EQ(solver.currentValue(counts.at(i)),
                          expectedCounts.at(i));
              }
            } else {
              const auto& [sat, expectedCounts] =
                  compute(inputVals, cover, countVals);
              if (Type == ViolationInvariantType::REIFIED) {
                EXPECT_EQ(solver.currentValue(violationId) == 0, sat);
              } else {
                bool actual = solver.currentValue(violationId) == 0;
                EXPECT_NE(actual, sat);
              }
            }
          }
        }
      }
    }
  }
};

class GlobalCardinalityClosedNodeTest
    : public AbstractGlobalCardinalityClosedNodeTest<ViolationInvariantType::NORMAL> {};

TEST_F(GlobalCardinalityClosedNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityClosedNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityClosedNodeTest, Propagation) { propagation(); }

class GlobalCardinalityClosedReifNodeTest
    : public AbstractGlobalCardinalityClosedNodeTest<ViolationInvariantType::REIFIED> {
};

TEST_F(GlobalCardinalityClosedReifNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityClosedReifNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityClosedReifNodeTest, Propagation) { propagation(); }

class GlobalCardinalityClosedFalseNodeTest
    : public AbstractGlobalCardinalityClosedNodeTest<
          ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(GlobalCardinalityClosedFalseNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityClosedFalseNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityClosedFalseNodeTest, Propagation) { propagation(); }

class GlobalCardinalityClosedTrueNodeTest
    : public AbstractGlobalCardinalityClosedNodeTest<
          ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(GlobalCardinalityClosedTrueNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityClosedTrueNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityClosedTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing