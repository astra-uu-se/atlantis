#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/countLtNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

// Constrains c to be strictly less than the number of occurrences of y in x.
static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  Int occurrences = 0;
  for (const Int val : values) {
    occurrences += (val == y ? 1 : 0);
  }
  return c < occurrences;
}

template <bool YIsParameter, ViolationInvariantType Type>
class AbstractCountLtNodeTest : public NodeTestBase<CountLtNode> {
 public:
  VarNodeId x1;
  VarNodeId x2;
  VarNodeId x3;
  VarNodeId y;
  VarNodeId c;
  VarNodeId r;
  const Int yParamVal{5};
  const Int cParamVal{2};

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVar(2, 5, "x1");
    x2 = createIntVar(3, 5, "x2");
    x3 = createIntVar(4, 5, "x3");
    y = createIntVar(2, 5, "y");
    c = createIntVar(0, 2, "c");
    addFznVar("r");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(x1));
    inputs.append(intVar(x2));
    inputs.append(intVar(x3));

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      if constexpr (YIsParameter) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_lt_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_lt_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));
      }
    } else {
      // No variable reification:
      if constexpr (Type == ViolationInvariantType::NORMAL) {
        if constexpr (YIsParameter) {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_lt",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                          fznparser::IntArg{intVar(c)}})));

        } else {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_lt",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                          fznparser::IntArg{intVar(c)}})));
        }
      } else {
        // constant reification:
        if constexpr (YIsParameter) {
          if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_lt_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_lt_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{true}})));
          }
        } else {
          if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_lt_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{intVar(y)},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_lt_reif",
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
    if constexpr (YIsParameter) {
      EXPECT_EQ(invNode().yVarNode(), NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().yVarNode(), NULL_NODE_ID);
      expectedInputs.emplace_back(y);
    }
    if constexpr (Type != ViolationInvariantType::NORMAL) {
      EXPECT_EQ(invNode().cVarNode(), NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().cVarNode(), NULL_NODE_ID);
      expectedInputs.emplace_back(c);
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    std::vector<VarNodeId> expectedOutputs;
    if constexpr (Type == ViolationInvariantType::REIFIED) {
      expectedOutputs.emplace_back(r);
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
    expectInputTo(invNode());
    expectOutputOf(invNode());

    std::vector<VarNodeId> expectedInputs{x1, x2, x3};
    if constexpr (YIsParameter) {
      EXPECT_EQ(invNode().yVarNode(), NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().yVarNode(), NULL_NODE_ID);
      expectedInputs.emplace_back(y);
    }
    if constexpr (Type != ViolationInvariantType::NORMAL) {
      EXPECT_EQ(invNode().cVarNode(), NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().cVarNode(), NULL_NODE_ID);
      expectedInputs.emplace_back(c);
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    std::vector<VarNodeId> expectedOutputs;
    if constexpr (Type == ViolationInvariantType::REIFIED) {
      expectedOutputs.emplace_back(r);
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

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVars(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputs;
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }

    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);

    std::vector<Int> values;
    Int yLb = yParamVal;
    Int yUb = yParamVal;
    Int cLb = cParamVal;
    Int cUb = cParamVal;

    values.resize(3);
    const propagation::VarId yVar = invNode().yVarNode() == NULL_NODE_ID
                                        ? propagation::NULL_ID
                                        : varId(invNode().yVarNode());

    if constexpr (!YIsParameter) {
      EXPECT_NE(yVar, propagation::NULL_ID);
      yLb = solver.lowerBound(yVar);
      yUb = solver.upperBound(yVar);
    }

    const propagation::VarId cVar = invNode().cVarNode() == NULL_NODE_ID
                                        ? propagation::NULL_ID
                                        : varId(invNode().cVarNode());
    if constexpr (Type == ViolationInvariantType::NORMAL) {
      EXPECT_NE(cVar, propagation::NULL_ID);
      cLb = solver.lowerBound(cVar);
      cUb = solver.upperBound(cVar);
    }
    solver.close();

    for (values.at(0) = solver.lowerBound(inputs.at(0));
         values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = solver.lowerBound(inputs.at(1));
           values.at(1) <= solver.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = solver.lowerBound(inputs.at(2));
             values.at(2) <= solver.upperBound(inputs.at(2)); ++values.at(2)) {
          for (Int yVal = yLb; yVal <= yUb; ++yVal) {
            for (Int cVal = cLb; cVal <= cUb; ++cVal) {
              solver.beginMove();
              for (size_t i = 0; i < values.size(); ++i) {
                solver.setValue(inputs.at(i), values.at(i));
              }
              if constexpr (!YIsParameter) {
                solver.setValue(yVar, yVal);
              }
              if constexpr (Type == ViolationInvariantType::NORMAL) {
                solver.setValue(cVar, cVal);
              }
              solver.endMove();

              solver.beginProbe();
              solver.query(violationId);
              solver.endProbe();
              if constexpr (!YIsParameter) {
                EXPECT_EQ(solver.currentValue(yVar), yVal);
              }
              if constexpr (Type == ViolationInvariantType::NORMAL) {
                EXPECT_EQ(solver.currentValue(cVar), cVal);
              }

              if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
                const bool actual = solver.currentValue(violationId) == 0;
                const bool expected = isSatisfied(values, yVal, cVal);
                EXPECT_EQ(actual, expected);
              } else {
                EXPECT_NE(solver.currentValue(violationId) == 0,
                          isSatisfied(values, yVal, cVal));
              }
            }
          }
        }
      }
    }
  }
};

class CountLtNodeTest
    : public AbstractCountLtNodeTest<false, ViolationInvariantType::NORMAL> {};

TEST_F(CountLtNodeTest, Construction) { construction(); }

TEST_F(CountLtNodeTest, Application) { application(); }

TEST_F(CountLtNodeTest, Propagation) { propagation(); }

class CountLtReifNodeTest
    : public AbstractCountLtNodeTest<false, ViolationInvariantType::REIFIED> {};

TEST_F(CountLtReifNodeTest, Construction) { construction(); }

TEST_F(CountLtReifNodeTest, Application) { application(); }

TEST_F(CountLtReifNodeTest, Propagation) { propagation(); }

class CountLtFalseNodeTest
    : public AbstractCountLtNodeTest<false, ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(CountLtFalseNodeTest, Construction) { construction(); }

TEST_F(CountLtFalseNodeTest, Application) { application(); }

TEST_F(CountLtFalseNodeTest, Propagation) { propagation(); }

class CountLtTrueNodeTest
    : public AbstractCountLtNodeTest<false, ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(CountLtTrueNodeTest, Construction) { construction(); }

TEST_F(CountLtTrueNodeTest, Application) { application(); }

TEST_F(CountLtTrueNodeTest, Propagation) { propagation(); }

class CountLtYParNodeTest
    : public AbstractCountLtNodeTest<true, ViolationInvariantType::NORMAL> {};

TEST_F(CountLtYParNodeTest, Construction) { construction(); }

TEST_F(CountLtYParNodeTest, Application) { application(); }

TEST_F(CountLtYParNodeTest, Propagation) { propagation(); }

class CountLtYParReifNodeTest
    : public AbstractCountLtNodeTest<true, ViolationInvariantType::REIFIED> {};

TEST_F(CountLtYParReifNodeTest, Construction) { construction(); }

TEST_F(CountLtYParReifNodeTest, Application) { application(); }

TEST_F(CountLtYParReifNodeTest, Propagation) { propagation(); }

class CountLtYParFalseNodeTest
    : public AbstractCountLtNodeTest<true, ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(CountLtYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountLtYParFalseNodeTest, Application) { application(); }

TEST_F(CountLtYParFalseNodeTest, Propagation) { propagation(); }

class CountLtYParTrueNodeTest
    : public AbstractCountLtNodeTest<true, ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(CountLtYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountLtYParTrueNodeTest, Application) { application(); }

TEST_F(CountLtYParTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing