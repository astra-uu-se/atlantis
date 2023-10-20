#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/countLtNode.hpp"

// Constrains c to be strictly less than the number of occurrences of y in x.
static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  Int occurrences = 0;
  for (const Int val : values) {
    occurrences += (val == y ? 1 : 0);
  }
  return c < occurrences;
}

template <bool YIsParameter, ConstraintType Type>
class AbstractCountLtNodeTest
    : public NodeTestBase<invariantgraph::CountLtNode> {
 public:
  invariantgraph::VarNodeId x1;
  invariantgraph::VarNodeId x2;
  invariantgraph::VarNodeId x3;
  invariantgraph::VarNodeId y;
  invariantgraph::VarNodeId c;
  invariantgraph::VarNodeId r;
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
      if constexpr (Type == ConstraintType::NORMAL) {
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
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
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
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
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

    std::vector<invariantgraph::VarNodeId> expectedInputs{x1, x2, x3};
    if constexpr (YIsParameter) {
      EXPECT_EQ(invNode().yVarNode(), invariantgraph::NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().yVarNode(), invariantgraph::NULL_NODE_ID);
      expectedInputs.emplace_back(y);
    }
    if constexpr (Type != ConstraintType::NORMAL) {
      EXPECT_EQ(invNode().cVarNode(), invariantgraph::NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().cVarNode(), invariantgraph::NULL_NODE_ID);
      expectedInputs.emplace_back(c);
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs,
                testing::ContainerEq(invNode().staticInputVarNodeIds()));

    std::vector<invariantgraph::VarNodeId> expectedOutputs;
    if constexpr (Type == ConstraintType::REIFIED) {
      expectedOutputs.emplace_back(r);
    }
    EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
    EXPECT_THAT(expectedOutputs,
                testing::ContainerEq(invNode().outputVarNodeIds()));

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
    expectInputTo(invNode());
    expectOutputOf(invNode());

    std::vector<invariantgraph::VarNodeId> expectedInputs{x1, x2, x3};
    if constexpr (YIsParameter) {
      EXPECT_EQ(invNode().yVarNode(), invariantgraph::NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().yVarNode(), invariantgraph::NULL_NODE_ID);
      expectedInputs.emplace_back(y);
    }
    if constexpr (Type != ConstraintType::NORMAL) {
      EXPECT_EQ(invNode().cVarNode(), invariantgraph::NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().cVarNode(), invariantgraph::NULL_NODE_ID);
      expectedInputs.emplace_back(c);
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs,
                testing::ContainerEq(invNode().staticInputVarNodeIds()));

    std::vector<invariantgraph::VarNodeId> expectedOutputs;
    if constexpr (Type == ConstraintType::REIFIED) {
      expectedOutputs.emplace_back(r);
    }

    EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
    EXPECT_THAT(expectedOutputs,
                testing::ContainerEq(invNode().outputVarNodeIds()));

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

    const VarId violationId = invNode().violationVarId(*_invariantGraph);

    std::vector<Int> values;
    Int yLb = yParamVal;
    Int yUb = yParamVal;
    Int cLb = cParamVal;
    Int cUb = cParamVal;

    values.resize(3);
    const VarId yVar = invNode().yVarNode() == invariantgraph::NULL_NODE_ID
                           ? NULL_ID
                           : varId(invNode().yVarNode());

    if constexpr (!YIsParameter) {
      EXPECT_NE(yVar, NULL_ID);
      yLb = engine.lowerBound(yVar);
      yUb = engine.upperBound(yVar);
    }

    const VarId cVar = invNode().cVarNode() == invariantgraph::NULL_NODE_ID
                           ? NULL_ID
                           : varId(invNode().cVarNode());
    if constexpr (Type == ConstraintType::NORMAL) {
      EXPECT_NE(cVar, NULL_ID);
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

class CountLtNodeTest
    : public AbstractCountLtNodeTest<false, ConstraintType::NORMAL> {};

TEST_F(CountLtNodeTest, Construction) { construction(); }

TEST_F(CountLtNodeTest, Application) { application(); }

TEST_F(CountLtNodeTest, Propagation) { propagation(); }

class CountLtReifNodeTest
    : public AbstractCountLtNodeTest<false, ConstraintType::REIFIED> {};

TEST_F(CountLtReifNodeTest, Construction) { construction(); }

TEST_F(CountLtReifNodeTest, Application) { application(); }

TEST_F(CountLtReifNodeTest, Propagation) { propagation(); }

class CountLtFalseNodeTest
    : public AbstractCountLtNodeTest<false, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountLtFalseNodeTest, Construction) { construction(); }

TEST_F(CountLtFalseNodeTest, Application) { application(); }

TEST_F(CountLtFalseNodeTest, Propagation) { propagation(); }

class CountLtTrueNodeTest
    : public AbstractCountLtNodeTest<false, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountLtTrueNodeTest, Construction) { construction(); }

TEST_F(CountLtTrueNodeTest, Application) { application(); }

TEST_F(CountLtTrueNodeTest, Propagation) { propagation(); }

class CountLtYParNodeTest
    : public AbstractCountLtNodeTest<true, ConstraintType::NORMAL> {};

TEST_F(CountLtYParNodeTest, Construction) { construction(); }

TEST_F(CountLtYParNodeTest, Application) { application(); }

TEST_F(CountLtYParNodeTest, Propagation) { propagation(); }

class CountLtYParReifNodeTest
    : public AbstractCountLtNodeTest<true, ConstraintType::REIFIED> {};

TEST_F(CountLtYParReifNodeTest, Construction) { construction(); }

TEST_F(CountLtYParReifNodeTest, Application) { application(); }

TEST_F(CountLtYParReifNodeTest, Propagation) { propagation(); }

class CountLtYParFalseNodeTest
    : public AbstractCountLtNodeTest<true, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountLtYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountLtYParFalseNodeTest, Application) { application(); }

TEST_F(CountLtYParFalseNodeTest, Propagation) { propagation(); }

class CountLtYParTrueNodeTest
    : public AbstractCountLtNodeTest<true, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountLtYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountLtYParTrueNodeTest, Application) { application(); }

TEST_F(CountLtYParTrueNodeTest, Propagation) { propagation(); }