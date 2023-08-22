#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/countGtNode.hpp"

static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  Int count = 0;
  for (const Int val : values) {
    count += static_cast<Int>(val == y);
  }
  return c > count;
}

template <bool YIsParameter, bool CIsParameter, ConstraintType Type>
class AbstractCountGtNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> x1;
  std::unique_ptr<fznparser::IntVar> x2;
  std::unique_ptr<fznparser::IntVar> x3;
  std::unique_ptr<fznparser::IntVar> y;
  std::unique_ptr<fznparser::IntVar> c;
  std::unique_ptr<fznparser::BoolVar> r;
  const Int yParamVal{5};
  const Int cParamVal{2};

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;
  std::unique_ptr<invariantgraph::CountGtNode> node;

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
            "fzn_count_gt_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{*r}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_gt_reif",
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
                std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                            fznparser::IntArg{cParamVal},
                                            fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_gt_reif",
                std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                            fznparser::IntArg{cParamVal},
                                            fznparser::BoolArg{true}})));
          }
        }
      }
    }

    node = makeNode<invariantgraph::CountGtNode>(_model->constraints().front());
  }

  void construction() {
    size_t inputSize = 5;
    if constexpr (YIsParameter) {
      --inputSize;
      EXPECT_EQ(node->yVarNode(), nullptr);
    } else {
      EXPECT_NE(node->yVarNode(), nullptr);
    }
    if constexpr (CIsParameter) {
      --inputSize;
      EXPECT_EQ(node->cVarNode(), nullptr);
    } else {
      EXPECT_NE(node->cVarNode(), nullptr);
    }
    EXPECT_EQ(node->staticInputVarNodeIds().size(), inputSize);
    std::vector<invariantgraph::VarNodeId> expectedInputs{
        _nodeMap->at("x1"), _nodeMap->at("x2"), _nodeMap->at("c"),
        _nodeMap->at("y")};
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
    EXPECT_NE(node->violationVarId(), NULL_ID);
    node->registerNode(*_invariantGraph, engine);
    engine.close();

    // x1, x2, x3, y, c
    size_t numSearchVars = 5;
    // x1, x2, x3, y, c, the violation, dummy objective
    size_t numVariables = 7;

    if constexpr (YIsParameter) {
      --numSearchVars;
      --numVariables;
    }
    if constexpr (CIsParameter) {
      --numSearchVars;
      --numVariables;
    }
    EXPECT_EQ(engine.searchVariables().size(), numSearchVars);
    // x1, x2, x3, and the violation
    EXPECT_EQ(engine.numVariables(), numVariables);

    // countEq
    size_t numInvariants = 2;
    if constexpr (CIsParameter) {
      --numInvariants;
    }

    EXPECT_EQ(engine.numInvariants(), numInvariants);

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
    size_t numStaticInputs = 5;
    if constexpr (YIsParameter) {
      --numStaticInputs;
    }
    if constexpr (CIsParameter) {
      --numStaticInputs;
    }
    EXPECT_EQ(node->staticInputVarNodeIds().size(), numStaticInputs);

    for (auto* const inputVariable : node->staticInputVarNodeIds()) {
      EXPECT_NE(inputVariable->varId(), NULL_ID);
      inputs.emplace_back(inputVariable->varId());
    }
    EXPECT_EQ(inputs.size(), numStaticInputs);

    const VarId violationId = node->violationVarId();

    std::vector<Int> values;
    Int yLb = yParamVal;
    Int yUb = yParamVal;
    Int cLb = cParamVal;
    Int cUb = cParamVal;

    values.resize(3);
    const VarId yVar =
        node->yVarNode() == nullptr ? NULL_ID : node->yVarNode()->varId();

    if constexpr (!YIsParameter) {
      EXPECT_NE(yVar, NULL_ID);
      yLb = engine.lowerBound(yVar);
      yUb = engine.upperBound(yVar);
    }

    const VarId cVar =
        node->cVarNode() == nullptr ? NULL_ID : node->cVarNode()->varId();
    if constexpr (!CIsParameter) {
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
              if constexpr (!CIsParameter) {
                engine.setValue(cVar, cVal);
              }
              engine.endMove();

              engine.beginProbe();
              engine.query(violationId);
              engine.endProbe();
              if constexpr (!YIsParameter) {
                EXPECT_EQ(engine.currentValue(yVar), yVal);
              }
              if constexpr (!CIsParameter) {
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
    : public AbstractCountGtNodeTest<false, false, ConstraintType::NORMAL> {};

TEST_F(CountGtNodeTest, Construction) { construction(); }

TEST_F(CountGtNodeTest, Application) { application(); }

TEST_F(CountGtNodeTest, Propagation) { propagation(); }

class CountGtReifNodeTest
    : public AbstractCountGtNodeTest<false, false, ConstraintType::REIFIED> {};

TEST_F(CountGtReifNodeTest, Construction) { construction(); }

TEST_F(CountGtReifNodeTest, Application) { application(); }

TEST_F(CountGtReifNodeTest, Propagation) { propagation(); }

class CountGtFalseNodeTest
    : public AbstractCountGtNodeTest<false, false,
                                     ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountGtFalseNodeTest, Construction) { construction(); }

TEST_F(CountGtFalseNodeTest, Application) { application(); }

TEST_F(CountGtFalseNodeTest, Propagation) { propagation(); }

class CountGtTrueNodeTest
    : public AbstractCountGtNodeTest<false, false,
                                     ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountGtTrueNodeTest, Construction) { construction(); }

TEST_F(CountGtTrueNodeTest, Application) { application(); }

TEST_F(CountGtTrueNodeTest, Propagation) { propagation(); }

class CountGtYParNodeTest
    : public AbstractCountGtNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountGtYParNodeTest, Construction) { construction(); }

TEST_F(CountGtYParNodeTest, Application) { application(); }

TEST_F(CountGtYParNodeTest, Propagation) { propagation(); }

class CountGtYParReifNodeTest
    : public AbstractCountGtNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountGtYParReifNodeTest, Construction) { construction(); }

TEST_F(CountGtYParReifNodeTest, Application) { application(); }

TEST_F(CountGtYParReifNodeTest, Propagation) { propagation(); }

class CountGtYParFalseNodeTest
    : public AbstractCountGtNodeTest<true, false,
                                     ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountGtYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountGtYParFalseNodeTest, Application) { application(); }

TEST_F(CountGtYParFalseNodeTest, Propagation) { propagation(); }

class CountGtYParTrueNodeTest
    : public AbstractCountGtNodeTest<true, false,
                                     ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountGtYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountGtYParTrueNodeTest, Application) { application(); }

TEST_F(CountGtYParTrueNodeTest, Propagation) { propagation(); }

class CountGtCParNodeTest
    : public AbstractCountGtNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountGtCParNodeTest, Construction) { construction(); }

TEST_F(CountGtCParNodeTest, Application) { application(); }

TEST_F(CountGtCParNodeTest, Propagation) { propagation(); }

class CountGtCParReifNodeTest
    : public AbstractCountGtNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountGtCParReifNodeTest, Construction) { construction(); }

TEST_F(CountGtCParReifNodeTest, Application) { application(); }

TEST_F(CountGtCParReifNodeTest, Propagation) { propagation(); }

class CountGtCParFalseNodeTest
    : public AbstractCountGtNodeTest<true, false,
                                     ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountGtCParFalseNodeTest, Construction) { construction(); }

TEST_F(CountGtCParFalseNodeTest, Application) { application(); }

TEST_F(CountGtCParFalseNodeTest, Propagation) { propagation(); }

class CountGtCParTrueNodeTest
    : public AbstractCountGtNodeTest<true, false,
                                     ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountGtCParTrueNodeTest, Construction) { construction(); }

TEST_F(CountGtCParTrueNodeTest, Application) { application(); }

TEST_F(CountGtCParTrueNodeTest, Propagation) { propagation(); }

class CountGtYParCParNodeTest
    : public AbstractCountGtNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountGtYParCParNodeTest, Construction) { construction(); }

TEST_F(CountGtYParCParNodeTest, Application) { application(); }

TEST_F(CountGtYParCParNodeTest, Propagation) { propagation(); }

class CountGtYParCParReifNodeTest
    : public AbstractCountGtNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountGtYParCParReifNodeTest, Construction) { construction(); }

TEST_F(CountGtYParCParReifNodeTest, Application) { application(); }

TEST_F(CountGtYParCParReifNodeTest, Propagation) { propagation(); }

class CountGtYParCParFalseNodeTest
    : public AbstractCountGtNodeTest<true, false,
                                     ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountGtYParCParFalseNodeTest, Construction) { construction(); }

TEST_F(CountGtYParCParFalseNodeTest, Application) { application(); }

TEST_F(CountGtYParCParFalseNodeTest, Propagation) { propagation(); }

class CountGtYParCParTrueNodeTest
    : public AbstractCountGtNodeTest<true, false,
                                     ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountGtYParCParTrueNodeTest, Construction) { construction(); }

TEST_F(CountGtYParCParTrueNodeTest, Application) { application(); }

TEST_F(CountGtYParCParTrueNodeTest, Propagation) { propagation(); }