#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/countGtNode.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

// Constrains c to be strictly greater than the number of occurrences of y in x.
static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  Int occurrences = 0;
  for (const Int val : values) {
    occurrences += (val == y ? 1 : 0);
  }
  return c > occurrences;
}

template <bool YIsParameter, ConstraintType Type>
class AbstractCountGtNodeTest : public NodeTestBase<CountGtNode> {
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
    r = createBoolVar("r");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(x1));
    inputs.append(intVar(x2));
    inputs.append(intVar(x3));

    if constexpr (Type == ConstraintType::REIFIED) {
      if constexpr (YIsParameter) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_gt_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_gt_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));
      }
    } else {
      // No variable reification:
      if constexpr (Type == ConstraintType::NORMAL) {
        if constexpr (YIsParameter) {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_gt",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                          fznparser::IntArg{intVar(c)}})));

        } else {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_gt",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                          fznparser::IntArg{intVar(c)}})));
        }
      } else {
        // constant reification:
        if constexpr (YIsParameter) {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_gt_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_gt_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{true}})));
          }
        } else {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_gt_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{intVar(y)},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_gt_reif",
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
      expectedInputs.emplace_back(y);
      EXPECT_NE(invNode().yVarNode(), NULL_NODE_ID);
    }
    if constexpr (Type != ConstraintType::NORMAL) {
      EXPECT_EQ(invNode().cVarNode(), NULL_NODE_ID);
    } else {
      expectedInputs.emplace_back(c);
      EXPECT_NE(invNode().cVarNode(), NULL_NODE_ID);
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    std::vector<VarNodeId> expectedOutputs;
    if constexpr (Type == ConstraintType::REIFIED) {
      expectedOutputs.emplace_back(r);
    }
    EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
    EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));

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
    expectInputTo(invNode());
    expectOutputOf(invNode());

    std::vector<VarNodeId> expectedInputs{x1, x2, x3};
    if constexpr (YIsParameter) {
      EXPECT_EQ(invNode().yVarNode(), NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().yVarNode(), NULL_NODE_ID);
      expectedInputs.emplace_back(y);
    }
    if constexpr (Type != ConstraintType::NORMAL) {
      EXPECT_EQ(invNode().cVarNode(), NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().cVarNode(), NULL_NODE_ID);
      expectedInputs.emplace_back(c);
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    std::vector<VarNodeId> expectedOutputs;
    if constexpr (Type == ConstraintType::REIFIED) {
      expectedOutputs.emplace_back(r);
    }

    EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
    EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));

    if constexpr (Type == ConstraintType::REIFIED) {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    } else {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    }
  }

  void propagation() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    invNode().registerNode(*_invariantGraph, engine);

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
      yLb = engine.lowerBound(yVar);
      yUb = engine.upperBound(yVar);
    }

    const propagation::VarId cVar = invNode().cVarNode() == NULL_NODE_ID
                                        ? propagation::NULL_ID
                                        : varId(invNode().cVarNode());
    if constexpr (Type == ConstraintType::NORMAL) {
      EXPECT_NE(cVar, propagation::NULL_ID);
      cLb = engine.lowerBound(cVar);
      cUb = engine.upperBound(cVar);
    }
    engine.close();

    for (values.at(0) = engine.lowerBound(inputs.at(0));
         values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = engine.lowerBound(inputs.at(1));
           values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = engine.lowerBound(inputs.at(2));
             values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
          for (Int yVal = yLb; yVal <= yUb; ++yVal) {
            for (Int cVal = cLb; cVal <= cUb; ++cVal) {
              engine.beginMove();
              for (size_t i = 0; i < values.size(); ++i) {
                engine.setValue(inputs.at(i), values.at(i));
              }
              if constexpr (!YIsParameter) {
                engine.setValue(yVar, yVal);
              }
              if constexpr (Type == ConstraintType::NORMAL) {
                engine.setValue(cVar, cVal);
              }
              engine.endMove();

              engine.beginProbe();
              engine.query(violationId);
              engine.endProbe();
              if constexpr (!YIsParameter) {
                EXPECT_EQ(engine.currentValue(yVar), yVal);
              }
              if constexpr (Type == ConstraintType::NORMAL) {
                EXPECT_EQ(engine.currentValue(cVar), cVal);
              }

              if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
                const bool actual = engine.currentValue(violationId) == 0;
                const bool expected = isSatisfied(values, yVal, cVal);
                EXPECT_EQ(actual, expected);
              } else {
                EXPECT_NE(engine.currentValue(violationId) == 0,
                          isSatisfied(values, yVal, cVal));
              }
            }
          }
        }
      }
    }
  }
};

class CountGtNodeTest
    : public AbstractCountGtNodeTest<false, ConstraintType::NORMAL> {};

TEST_F(CountGtNodeTest, Construction) { construction(); }

TEST_F(CountGtNodeTest, Application) { application(); }

TEST_F(CountGtNodeTest, Propagation) { propagation(); }

class CountGtReifNodeTest
    : public AbstractCountGtNodeTest<false, ConstraintType::REIFIED> {};

TEST_F(CountGtReifNodeTest, Construction) { construction(); }

TEST_F(CountGtReifNodeTest, Application) { application(); }

TEST_F(CountGtReifNodeTest, Propagation) { propagation(); }

class CountGtFalseNodeTest
    : public AbstractCountGtNodeTest<false, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountGtFalseNodeTest, Construction) { construction(); }

TEST_F(CountGtFalseNodeTest, Application) { application(); }

TEST_F(CountGtFalseNodeTest, Propagation) { propagation(); }

class CountGtTrueNodeTest
    : public AbstractCountGtNodeTest<false, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountGtTrueNodeTest, Construction) { construction(); }

TEST_F(CountGtTrueNodeTest, Application) { application(); }

TEST_F(CountGtTrueNodeTest, Propagation) { propagation(); }

class CountGtYParNodeTest
    : public AbstractCountGtNodeTest<true, ConstraintType::NORMAL> {};

TEST_F(CountGtYParNodeTest, Construction) { construction(); }

TEST_F(CountGtYParNodeTest, Application) { application(); }

TEST_F(CountGtYParNodeTest, Propagation) { propagation(); }

class CountGtYParReifNodeTest
    : public AbstractCountGtNodeTest<true, ConstraintType::REIFIED> {};

TEST_F(CountGtYParReifNodeTest, Construction) { construction(); }

TEST_F(CountGtYParReifNodeTest, Application) { application(); }

TEST_F(CountGtYParReifNodeTest, Propagation) { propagation(); }

class CountGtYParFalseNodeTest
    : public AbstractCountGtNodeTest<true, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountGtYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountGtYParFalseNodeTest, Application) { application(); }

TEST_F(CountGtYParFalseNodeTest, Propagation) { propagation(); }

class CountGtYParTrueNodeTest
    : public AbstractCountGtNodeTest<true, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountGtYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountGtYParTrueNodeTest, Application) { application(); }

TEST_F(CountGtYParTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing