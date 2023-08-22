#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/countEqNode.hpp"

static Int computeOutput(const std::vector<Int>& values, const Int y) {
  Int count = 0;
  for (const Int val : values) {
    count += static_cast<Int>(val == y);
  }
  return count;
}

static bool isViolating(const std::vector<Int>& values, const Int y,
                        const Int c) {
  return computeOutput(values, y) != c;
}

template <bool YIsParameter, ConstraintType Type>
class AbstractCountEqNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> x1;
  std::unique_ptr<fznparser::IntVar> x2;
  std::unique_ptr<fznparser::IntVar> x3;
  std::unique_ptr<fznparser::IntVar> y;
  std::unique_ptr<fznparser::IntVar> c;
  std::unique_ptr<fznparser::BoolVar> r;
  const Int cParamVal{2};
  const Int yParamVal{5};

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;
  std::unique_ptr<invariantgraph::CountEqNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = intVar(5, 10, "x1");
    x2 = intVar(2, 7, "x2");
    x3 = intVar(2, 7, "x3");
    y = intVar(2, 7, "y");
    c = intVar(2, 7, "c");
    r = boolVar("r");

    fznparser::IntVarArray inputs("");
    inputs.append(*x1);
    inputs.append(*x2);
    inputs.append(*x3);

    if constexpr (Type == ConstraintType::REIFIED) {
      if constexpr (YIsParameter) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_eq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{*r}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_eq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{*r}})));
      }
    } else {
      // No variable reification:
      if constexpr (Type == ConstraintType::NORMAL) {
        if constexpr (YIsParameter) {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_eq",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                          fznparser::IntArg{*c}})));

        } else {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_eq",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                          fznparser::IntArg{*c}})));
        }
      } else {
        // constant reification:
        if constexpr (YIsParameter) {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
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
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_eq_reif",
                std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                            fznparser::IntArg{cParamVal},
                                            fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_eq_reif",
                std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                            fznparser::IntArg{cParamVal},
                                            fznparser::BoolArg{true}})));
          }
        }
      }
    }

    node = makeNode<invariantgraph::CountEqNode>(_model->constraints().front());
  }

  void construction() {
    size_t inputSize = 4;
    if constexpr (YIsParameter) {
      inputSize = 3;
    }
    EXPECT_EQ(node->staticInputVarNodeIds().size(), inputSize);
    std::vector<invariantgraph::VarNodeId> expectedInputs{
        _nodeMap->at("x1"), _nodeMap->at("x2"), _nodeMap->at("x3")};
    EXPECT_EQ(node->staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs,
                testing::ContainerEq(node->staticInputVarNodeIds()));
    expectMarkedAsInput(node.get(), node->staticInputVarNodeIds());

    std::vector<invariantgraph::VarNodeId> expectedOutputs{_nodeMap->at("c")};

    EXPECT_EQ(node->outputVarNodeIds(), expectedOutputs);
    EXPECT_THAT(expectedOutputs,
                testing::ContainerEq(node->outputVarNodeIds()));

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
    if constexpr (Type == ConstraintType::NORMAL) {
      EXPECT_EQ(node->violationVarId(), NULL_ID);
    } else {
      EXPECT_NE(node->violationVarId(), NULL_ID);
    }
    node->registerNode(*_invariantGraph, engine);
    engine.close();

    if constexpr (YIsParameter) {
      // x1, x2, and x3
      EXPECT_EQ(engine.searchVariables().size(), 3);
      // x1, x2, x3, and the violation
      EXPECT_EQ(engine.numVariables(), 4);
    } else {
      // x1, x2, x3, and y
      EXPECT_EQ(engine.searchVariables().size(), 4);
      // x1, x2, x3, y, and (c or intermediate)
      EXPECT_EQ(engine.numVariables(), 5);
    }

    // countEq
    EXPECT_EQ(engine.numInvariants(), 1);

    if constexpr (Type != ConstraintType::NORMAL) {
      EXPECT_EQ(engine.lowerBound(node->violationVarId()), 0);
      EXPECT_GT(engine.upperBound(node->violationVarId()), 0);
    }

    for (auto* definedVariable : node->outputVarNodeIds()) {
      EXPECT_EQ(engine.lowerBound(definedVariable->varId()), 0);
      EXPECT_GT(engine.upperBound(definedVariable->varId()), 0);
    }
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    addVariablesToEngine(engine);
    node->registerOutputVariables(engine);
    node->registerNode(*_invariantGraph, engine);

    std::vector<VarId> inputs;
    if constexpr (YIsParameter) {
      EXPECT_EQ(node->staticInputVarNodeIds().size(), 3);
    } else {
      EXPECT_EQ(node->staticInputVarNodeIds().size(), 4);
    }
    for (auto* const inputVariable : node->staticInputVarNodeIds()) {
      EXPECT_NE(inputVariable->varId(), NULL_ID);
      inputs.emplace_back(inputVariable->varId());
    }
    if constexpr (YIsParameter) {
      EXPECT_EQ(inputs.size(), 3);
    } else {
      EXPECT_EQ(inputs.size(), 4);
    }

    if constexpr (Type == ConstraintType::NORMAL) {
      EXPECT_NE(node->cVarNode(), nullptr);
      EXPECT_NE(node->cVarNode()->varId(), NULL_ID);
      EXPECT_EQ(node->violationVarId(), NULL_ID);
    } else {
      EXPECT_EQ(node->cVarNode(), nullptr);
      EXPECT_NE(node->violationVarId(), NULL_ID);
    }

    const VarId outputId =
        node->cVarNode() == nullptr ? NULL_ID : node->cVarNode()->varId();
    const VarId violationId = node->violationVarId();

    std::vector<Int> values;
    Int yLb = yParamVal;
    Int yUb = yParamVal;

    if constexpr (YIsParameter) {
      values.resize(inputs.size());
    } else {
      values.resize(inputs.size() - 1);
      yLb = engine.lowerBound(inputs.at(3));
      yUb = engine.upperBound(inputs.at(3));
    }
    engine.close();

    for (values.at(0) = engine.lowerBound(inputs.at(0));
         values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = engine.lowerBound(inputs.at(1));
           values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = engine.lowerBound(inputs.at(2));
             values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
          for (Int yVal = yLb; yVal <= yUb; ++yVal) {
            engine.beginMove();
            for (size_t i = 0; i < values.size(); ++i) {
              engine.setValue(inputs.at(i), values.at(i));
            }
            if constexpr (!YIsParameter) {
              engine.setValue(inputs.back(), yVal);
            }
            engine.endMove();

            engine.beginProbe();
            if (outputId != NULL_ID) {
              engine.query(outputId);
            }
            if (violationId != NULL_ID) {
              engine.query(violationId);
            }
            engine.endProbe();

            if (outputId != NULL_ID) {
              EXPECT_EQ(engine.currentValue(outputId),
                        computeOutput(values, yVal));
            }
            if (violationId != NULL_ID) {
              if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
                EXPECT_EQ(engine.currentValue(violationId) > 0,
                          isViolating(values, yVal, cParamVal));
              } else {
                EXPECT_NE(engine.currentValue(violationId) > 0,
                          isViolating(values, yVal, cParamVal));
              }
            }
          }
        }
      }
    }
  }
};

class CountEqNodeTest
    : public AbstractCountEqNodeTest<false, ConstraintType::NORMAL> {};

TEST_F(CountEqNodeTest, Construction) { construction(); }

TEST_F(CountEqNodeTest, Application) { application(); }

TEST_F(CountEqNodeTest, Propagation) { propagation(); }

class CountEqReifNodeTest
    : public AbstractCountEqNodeTest<false, ConstraintType::REIFIED> {};

TEST_F(CountEqReifNodeTest, Construction) { construction(); }

TEST_F(CountEqReifNodeTest, Application) { application(); }

TEST_F(CountEqReifNodeTest, Propagation) { propagation(); }

class CountEqFalseNodeTest
    : public AbstractCountEqNodeTest<false, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountEqFalseNodeTest, Construction) { construction(); }

TEST_F(CountEqFalseNodeTest, Application) { application(); }

TEST_F(CountEqFalseNodeTest, Propagation) { propagation(); }

class CountEqTrueNodeTest
    : public AbstractCountEqNodeTest<false, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountEqTrueNodeTest, Construction) { construction(); }

TEST_F(CountEqTrueNodeTest, Application) { application(); }

TEST_F(CountEqTrueNodeTest, Propagation) { propagation(); }

class CountEqYParNodeTest
    : public AbstractCountEqNodeTest<true, ConstraintType::NORMAL> {};

TEST_F(CountEqYParNodeTest, Construction) { construction(); }

TEST_F(CountEqYParNodeTest, Application) { application(); }

TEST_F(CountEqYParNodeTest, Propagation) { propagation(); }

class CountEqYParReifNodeTest
    : public AbstractCountEqNodeTest<true, ConstraintType::REIFIED> {};

TEST_F(CountEqYParReifNodeTest, Construction) { construction(); }

TEST_F(CountEqYParReifNodeTest, Application) { application(); }

TEST_F(CountEqYParReifNodeTest, Propagation) { propagation(); }

class CountEqYParFalseNodeTest
    : public AbstractCountEqNodeTest<true, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountEqYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountEqYParFalseNodeTest, Application) { application(); }

TEST_F(CountEqYParFalseNodeTest, Propagation) { propagation(); }

class CountEqYParTrueNodeTest
    : public AbstractCountEqNodeTest<true, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountEqYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountEqYParTrueNodeTest, Application) { application(); }

TEST_F(CountEqYParTrueNodeTest, Propagation) { propagation(); }