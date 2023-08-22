#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/countLtNode.hpp"

static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  Int count = 0;
  for (const Int val : values) {
    count += static_cast<Int>(val == y);
  }
  return c < count;
}

template <bool YIsParameter, bool CIsParameter, ConstraintType Type>
class AbstractCountLtNodeTest : public NodeTestBase {
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
  std::unique_ptr<invariantgraph::CountLtNode> node;

  void SetUp() override {
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
            "fzn_count_lt_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{*r}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_lt_reif",
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
                std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                            fznparser::IntArg{cParamVal},
                                            fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_lt_reif",
                std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                            fznparser::IntArg{cParamVal},
                                            fznparser::BoolArg{true}})));
          }
        }
      }
    }

    node = makeNode<invariantgraph::CountLtNode>(_model->constraints().front());
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

class CountLtNodeTest
    : public AbstractCountLtNodeTest<false, false, ConstraintType::NORMAL> {};

TEST_F(CountLtNodeTest, Construction) { construction(); }

TEST_F(CountLtNodeTest, Application) { application(); }

TEST_F(CountLtNodeTest, Propagation) { propagation(); }

class CountLtReifNodeTest
    : public AbstractCountLtNodeTest<false, false, ConstraintType::REIFIED> {};

TEST_F(CountLtReifNodeTest, Construction) { construction(); }

TEST_F(CountLtReifNodeTest, Application) { application(); }

TEST_F(CountLtReifNodeTest, Propagation) { propagation(); }

class CountLtFalseNodeTest
    : public AbstractCountLtNodeTest<false, false,
                                     ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountLtFalseNodeTest, Construction) { construction(); }

TEST_F(CountLtFalseNodeTest, Application) { application(); }

TEST_F(CountLtFalseNodeTest, Propagation) { propagation(); }

class CountLtTrueNodeTest
    : public AbstractCountLtNodeTest<false, false,
                                     ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountLtTrueNodeTest, Construction) { construction(); }

TEST_F(CountLtTrueNodeTest, Application) { application(); }

TEST_F(CountLtTrueNodeTest, Propagation) { propagation(); }

class CountLtYParNodeTest
    : public AbstractCountLtNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountLtYParNodeTest, Construction) { construction(); }

TEST_F(CountLtYParNodeTest, Application) { application(); }

TEST_F(CountLtYParNodeTest, Propagation) { propagation(); }

class CountLtYParReifNodeTest
    : public AbstractCountLtNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountLtYParReifNodeTest, Construction) { construction(); }

TEST_F(CountLtYParReifNodeTest, Application) { application(); }

TEST_F(CountLtYParReifNodeTest, Propagation) { propagation(); }

class CountLtYParFalseNodeTest
    : public AbstractCountLtNodeTest<true, false,
                                     ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountLtYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountLtYParFalseNodeTest, Application) { application(); }

TEST_F(CountLtYParFalseNodeTest, Propagation) { propagation(); }

class CountLtYParTrueNodeTest
    : public AbstractCountLtNodeTest<true, false,
                                     ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountLtYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountLtYParTrueNodeTest, Application) { application(); }

TEST_F(CountLtYParTrueNodeTest, Propagation) { propagation(); }

class CountLtCParNodeTest
    : public AbstractCountLtNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountLtCParNodeTest, Construction) { construction(); }

TEST_F(CountLtCParNodeTest, Application) { application(); }

TEST_F(CountLtCParNodeTest, Propagation) { propagation(); }

class CountLtCParReifNodeTest
    : public AbstractCountLtNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountLtCParReifNodeTest, Construction) { construction(); }

TEST_F(CountLtCParReifNodeTest, Application) { application(); }

TEST_F(CountLtCParReifNodeTest, Propagation) { propagation(); }

class CountLtCParFalseNodeTest
    : public AbstractCountLtNodeTest<true, false,
                                     ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountLtCParFalseNodeTest, Construction) { construction(); }

TEST_F(CountLtCParFalseNodeTest, Application) { application(); }

TEST_F(CountLtCParFalseNodeTest, Propagation) { propagation(); }

class CountLtCParTrueNodeTest
    : public AbstractCountLtNodeTest<true, false,
                                     ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountLtCParTrueNodeTest, Construction) { construction(); }

TEST_F(CountLtCParTrueNodeTest, Application) { application(); }

TEST_F(CountLtCParTrueNodeTest, Propagation) { propagation(); }

class CountLtYParCParNodeTest
    : public AbstractCountLtNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountLtYParCParNodeTest, Construction) { construction(); }

TEST_F(CountLtYParCParNodeTest, Application) { application(); }

TEST_F(CountLtYParCParNodeTest, Propagation) { propagation(); }

class CountLtYParCParReifNodeTest
    : public AbstractCountLtNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountLtYParCParReifNodeTest, Construction) { construction(); }

TEST_F(CountLtYParCParReifNodeTest, Application) { application(); }

TEST_F(CountLtYParCParReifNodeTest, Propagation) { propagation(); }

class CountLtYParCParFalseNodeTest
    : public AbstractCountLtNodeTest<true, false,
                                     ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountLtYParCParFalseNodeTest, Construction) { construction(); }

TEST_F(CountLtYParCParFalseNodeTest, Application) { application(); }

TEST_F(CountLtYParCParFalseNodeTest, Propagation) { propagation(); }

class CountLtYParCParTrueNodeTest
    : public AbstractCountLtNodeTest<true, false,
                                     ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountLtYParCParTrueNodeTest, Construction) { construction(); }

TEST_F(CountLtYParCParTrueNodeTest, Application) { application(); }

TEST_F(CountLtYParCParTrueNodeTest, Propagation) { propagation(); }