#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/countNeqNode.hpp"

static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  Int occurrences = 0;
  for (const Int val : values) {
    occurrences += (val == y ? 1 : 0);
  }
  return c != occurrences;
}

template <bool YIsParameter, ConstraintType Type>
class AbstractCountNeqNodeTest
    : public NodeTestBase<invariantgraph::CountNeqNode> {
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
            "fzn_count_neq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_neq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));
      }
    } else {
      // No variable reification:
      if constexpr (Type == ConstraintType::NORMAL) {
        if constexpr (YIsParameter) {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_neq",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                          fznparser::IntArg{intVar(c)}})));

        } else {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_neq",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                          fznparser::IntArg{intVar(c)}})));
        }
      } else {
        // constant reification:
        if constexpr (YIsParameter) {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_neq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_neq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{true}})));
          }
        } else {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_neq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{intVar(y)},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_neq_reif",
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
    size_t numStaticInputs = 5;
    if constexpr (YIsParameter) {
      --numStaticInputs;
    }
    if constexpr (Type != ConstraintType::NORMAL) {
      --numStaticInputs;
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numStaticInputs);

    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }
    EXPECT_EQ(inputs.size(), numStaticInputs);

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

class CountNeqNodeTest
    : public AbstractCountNeqNodeTest<false, ConstraintType::NORMAL> {};

TEST_F(CountNeqNodeTest, Construction) { construction(); }

TEST_F(CountNeqNodeTest, Application) { application(); }

TEST_F(CountNeqNodeTest, Propagation) { propagation(); }

class CountNeqReifNodeTest
    : public AbstractCountNeqNodeTest<false, ConstraintType::REIFIED> {};

TEST_F(CountNeqReifNodeTest, Construction) { construction(); }

TEST_F(CountNeqReifNodeTest, Application) { application(); }

TEST_F(CountNeqReifNodeTest, Propagation) { propagation(); }

class CountNeqFalseNodeTest
    : public AbstractCountNeqNodeTest<false, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountNeqFalseNodeTest, Construction) { construction(); }

TEST_F(CountNeqFalseNodeTest, Application) { application(); }

TEST_F(CountNeqFalseNodeTest, Propagation) { propagation(); }

class CountNeqTrueNodeTest
    : public AbstractCountNeqNodeTest<false, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountNeqTrueNodeTest, Construction) { construction(); }

TEST_F(CountNeqTrueNodeTest, Application) { application(); }

TEST_F(CountNeqTrueNodeTest, Propagation) { propagation(); }

class CountNeqYParNodeTest
    : public AbstractCountNeqNodeTest<true, ConstraintType::NORMAL> {};

TEST_F(CountNeqYParNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParNodeTest, Application) { application(); }

TEST_F(CountNeqYParNodeTest, Propagation) { propagation(); }

class CountNeqYParReifNodeTest
    : public AbstractCountNeqNodeTest<true, ConstraintType::REIFIED> {};

TEST_F(CountNeqYParReifNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParReifNodeTest, Application) { application(); }

TEST_F(CountNeqYParReifNodeTest, Propagation) { propagation(); }

class CountNeqYParFalseNodeTest
    : public AbstractCountNeqNodeTest<true, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountNeqYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParFalseNodeTest, Application) { application(); }

TEST_F(CountNeqYParFalseNodeTest, Propagation) { propagation(); }

class CountNeqYParTrueNodeTest
    : public AbstractCountNeqNodeTest<true, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountNeqYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParTrueNodeTest, Application) { application(); }

TEST_F(CountNeqYParTrueNodeTest, Propagation) { propagation(); }