#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/countEqNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static Int computeOutput(const std::vector<Int>& values, const Int y) {
  Int occurrences = 0;
  for (const Int val : values) {
    occurrences += (val == y ? 1 : 0);
  }
  return occurrences;
}

static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  return computeOutput(values, y) == c;
}

template <bool YIsParameter, ViolationInvariantType Type>
class AbstractCountEqNodeTest : public NodeTestBase<CountEqNode> {
 public:
  VarNodeId x1;
  VarNodeId x2;
  VarNodeId x3;
  VarNodeId y;
  VarNodeId c;
  VarNodeId r;
  const Int cParamVal{2};
  const Int yParamVal{5};

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVar(2, 5, "x1");
    x2 = createIntVar(3, 5, "x2");
    x3 = createIntVar(4, 5, "x3");
    y = createIntVar(2, 5, "y");
    c = createIntVar(0, 2, "c");
    r = createBoolVar("r");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(x1));
    inputs.append(intVar(x2));
    inputs.append(intVar(x3));

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      if constexpr (YIsParameter) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_eq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_eq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));
      }
    } else {
      // No variable reification:
      if constexpr (Type == ViolationInvariantType::NORMAL) {
        if constexpr (YIsParameter) {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_eq",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                          fznparser::IntArg{intVar(c)}})));

        } else {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_eq",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                          fznparser::IntArg{intVar(c)}})));
        }
      } else {
        // constant reification:
        if constexpr (YIsParameter) {
          if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_eq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_eq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{true}})));
          }
        } else {
          if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_eq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{intVar(y)},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_eq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{intVar(y)},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{true}})));
          }
        }
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    std::vector<VarNodeId> expectedInputs{x1, x2, x3};
    size_t inputSize = 3;
    if constexpr (!YIsParameter) {
      inputSize = 4;
      expectedInputs.emplace_back(y);
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputSize);

    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    std::vector<VarNodeId> expectedOutputs;
    if constexpr (Type == ViolationInvariantType::REIFIED) {
      expectedOutputs.emplace_back(r);
    } else if constexpr (Type == ViolationInvariantType::NORMAL) {
      expectedOutputs.emplace_back(c);
    }

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
    if constexpr (Type == ViolationInvariantType::NORMAL) {
      EXPECT_EQ(invNode().violationVarId(*_invariantGraph),
                propagation::NULL_ID);
    } else {
      EXPECT_NE(invNode().violationVarId(*_invariantGraph),
                propagation::NULL_ID);
    }
    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    if constexpr (YIsParameter) {
      // x1, x2, and x3
      EXPECT_EQ(solver.searchVars().size(), 3);
      // x1, x2, x3, and the violation
      EXPECT_EQ(solver.numVars(), 4);
    } else {
      // x1, x2, x3, and y
      EXPECT_EQ(solver.searchVars().size(), 4);
      // x1, x2, x3, y, and (c or intermediate)
      EXPECT_EQ(solver.numVars(), 5);
    }

    // countEq
    EXPECT_EQ(solver.numInvariants(), 1);

    if constexpr (Type != ViolationInvariantType::NORMAL) {
      EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)),
                0);
      EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)),
                0);
    }

    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(solver.lowerBound(varId(outputVarNodeId)), 0);
      EXPECT_GT(solver.upperBound(varId(outputVarNodeId)), 0);
    }
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVars(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputs;
    if constexpr (YIsParameter) {
      EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
    } else {
      EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 4);
    }
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }
    if constexpr (YIsParameter) {
      EXPECT_EQ(inputs.size(), 3);
    } else {
      EXPECT_EQ(inputs.size(), 4);
    }

    if constexpr (Type == ViolationInvariantType::NORMAL) {
      EXPECT_NE(invNode().cVarNode(), NULL_NODE_ID);
      EXPECT_NE(varId(invNode().cVarNode()), propagation::NULL_ID);
      EXPECT_EQ(invNode().violationVarId(*_invariantGraph),
                propagation::NULL_ID);
    } else {
      EXPECT_EQ(invNode().cVarNode(), NULL_NODE_ID);
      EXPECT_NE(invNode().violationVarId(*_invariantGraph),
                propagation::NULL_ID);
    }

    const propagation::VarId outputId = invNode().cVarNode() == NULL_NODE_ID
                                            ? propagation::NULL_ID
                                            : varId(invNode().cVarNode());
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);

    std::vector<Int> values;
    Int yLb = yParamVal;
    Int yUb = yParamVal;

    if constexpr (YIsParameter) {
      values.resize(inputs.size());
    } else {
      values.resize(inputs.size() - 1);
      yLb = solver.lowerBound(inputs.at(3));
      yUb = solver.upperBound(inputs.at(3));
    }
    solver.close();

    for (values.at(0) = solver.lowerBound(inputs.at(0));
         values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = solver.lowerBound(inputs.at(1));
           values.at(1) <= solver.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = solver.lowerBound(inputs.at(2));
             values.at(2) <= solver.upperBound(inputs.at(2)); ++values.at(2)) {
          for (Int yVal = yLb; yVal <= yUb; ++yVal) {
            solver.beginMove();
            for (size_t i = 0; i < values.size(); ++i) {
              solver.setValue(inputs.at(i), values.at(i));
            }
            if constexpr (!YIsParameter) {
              solver.setValue(inputs.back(), yVal);
            }
            solver.endMove();

            solver.beginProbe();
            if (outputId != propagation::NULL_ID) {
              solver.query(outputId);
            }
            if (violationId != propagation::NULL_ID) {
              solver.query(violationId);
            }
            solver.endProbe();

            if (outputId != propagation::NULL_ID) {
              EXPECT_EQ(solver.currentValue(outputId),
                        computeOutput(values, yVal));
            }
            if (violationId != propagation::NULL_ID) {
              if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
                EXPECT_EQ(solver.currentValue(violationId) == 0,
                          isSatisfied(values, yVal, cParamVal));
              } else {
                EXPECT_NE(solver.currentValue(violationId) == 0,
                          isSatisfied(values, yVal, cParamVal));
              }
            }
          }
        }
      }
    }
  }
};

class CountEqNodeTest
    : public AbstractCountEqNodeTest<false, ViolationInvariantType::NORMAL> {};

TEST_F(CountEqNodeTest, Construction) { construction(); }

TEST_F(CountEqNodeTest, Application) { application(); }

TEST_F(CountEqNodeTest, Propagation) { propagation(); }

class CountEqReifNodeTest
    : public AbstractCountEqNodeTest<false, ViolationInvariantType::REIFIED> {};

TEST_F(CountEqReifNodeTest, Construction) { construction(); }

TEST_F(CountEqReifNodeTest, Application) { application(); }

TEST_F(CountEqReifNodeTest, Propagation) { propagation(); }

class CountEqFalseNodeTest
    : public AbstractCountEqNodeTest<false, ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(CountEqFalseNodeTest, Construction) { construction(); }

TEST_F(CountEqFalseNodeTest, Application) { application(); }

TEST_F(CountEqFalseNodeTest, Propagation) { propagation(); }

class CountEqTrueNodeTest
    : public AbstractCountEqNodeTest<false, ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(CountEqTrueNodeTest, Construction) { construction(); }

TEST_F(CountEqTrueNodeTest, Application) { application(); }

TEST_F(CountEqTrueNodeTest, Propagation) { propagation(); }

class CountEqYParNodeTest
    : public AbstractCountEqNodeTest<true, ViolationInvariantType::NORMAL> {};

TEST_F(CountEqYParNodeTest, Construction) { construction(); }

TEST_F(CountEqYParNodeTest, Application) { application(); }

TEST_F(CountEqYParNodeTest, Propagation) { propagation(); }

class CountEqYParReifNodeTest
    : public AbstractCountEqNodeTest<true, ViolationInvariantType::REIFIED> {};

TEST_F(CountEqYParReifNodeTest, Construction) { construction(); }

TEST_F(CountEqYParReifNodeTest, Application) { application(); }

TEST_F(CountEqYParReifNodeTest, Propagation) { propagation(); }

class CountEqYParFalseNodeTest
    : public AbstractCountEqNodeTest<true, ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(CountEqYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountEqYParFalseNodeTest, Application) { application(); }

TEST_F(CountEqYParFalseNodeTest, Propagation) { propagation(); }

class CountEqYParTrueNodeTest
    : public AbstractCountEqNodeTest<true, ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(CountEqYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountEqYParTrueNodeTest, Application) { application(); }

TEST_F(CountEqYParTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing