#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/countNeqNode.hpp"

static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  Int count = 0;
  for (const Int val : values) {
    count += static_cast<Int>(val == y);
  }
  return c != count;
}

template <bool YIsParameter, bool CIsParameter, ConstraintType Type>
class AbstractCountNeqNodeTest : public NodeTestBase {
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
  std::unique_ptr<invariantgraph::CountNeqNode> node;

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
            "fzn_count_neq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{*r}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_neq_reif",
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
                std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                            fznparser::IntArg{cParamVal},
                                            fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_neq_reif",
                std::vector<fznparser::Arg>{inputs, fznparser::IntArg{*y},
                                            fznparser::IntArg{cParamVal},
                                            fznparser::BoolArg{true}})));
          }
        }
      }
    }

    node =
        makeNode<invariantgraph::CountNeqNode>(_model->constraints().front());
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

class CountNeqNodeTest
    : public AbstractCountNeqNodeTest<false, false, ConstraintType::NORMAL> {};

TEST_F(CountNeqNodeTest, Construction) { construction(); }

TEST_F(CountNeqNodeTest, Application) { application(); }

TEST_F(CountNeqNodeTest, Propagation) { propagation(); }

class CountNeqReifNodeTest
    : public AbstractCountNeqNodeTest<false, false, ConstraintType::REIFIED> {};

TEST_F(CountNeqReifNodeTest, Construction) { construction(); }

TEST_F(CountNeqReifNodeTest, Application) { application(); }

TEST_F(CountNeqReifNodeTest, Propagation) { propagation(); }

class CountNeqFalseNodeTest
    : public AbstractCountNeqNodeTest<false, false,
                                      ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountNeqFalseNodeTest, Construction) { construction(); }

TEST_F(CountNeqFalseNodeTest, Application) { application(); }

TEST_F(CountNeqFalseNodeTest, Propagation) { propagation(); }

class CountNeqTrueNodeTest
    : public AbstractCountNeqNodeTest<false, false,
                                      ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountNeqTrueNodeTest, Construction) { construction(); }

TEST_F(CountNeqTrueNodeTest, Application) { application(); }

TEST_F(CountNeqTrueNodeTest, Propagation) { propagation(); }

class CountNeqYParNodeTest
    : public AbstractCountNeqNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountNeqYParNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParNodeTest, Application) { application(); }

TEST_F(CountNeqYParNodeTest, Propagation) { propagation(); }

class CountNeqYParReifNodeTest
    : public AbstractCountNeqNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountNeqYParReifNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParReifNodeTest, Application) { application(); }

TEST_F(CountNeqYParReifNodeTest, Propagation) { propagation(); }

class CountNeqYParFalseNodeTest
    : public AbstractCountNeqNodeTest<true, false,
                                      ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountNeqYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParFalseNodeTest, Application) { application(); }

TEST_F(CountNeqYParFalseNodeTest, Propagation) { propagation(); }

class CountNeqYParTrueNodeTest
    : public AbstractCountNeqNodeTest<true, false,
                                      ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountNeqYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParTrueNodeTest, Application) { application(); }

TEST_F(CountNeqYParTrueNodeTest, Propagation) { propagation(); }

class CountNeqCParNodeTest
    : public AbstractCountNeqNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountNeqCParNodeTest, Construction) { construction(); }

TEST_F(CountNeqCParNodeTest, Application) { application(); }

TEST_F(CountNeqCParNodeTest, Propagation) { propagation(); }

class CountNeqCParReifNodeTest
    : public AbstractCountNeqNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountNeqCParReifNodeTest, Construction) { construction(); }

TEST_F(CountNeqCParReifNodeTest, Application) { application(); }

TEST_F(CountNeqCParReifNodeTest, Propagation) { propagation(); }

class CountNeqCParFalseNodeTest
    : public AbstractCountNeqNodeTest<true, false,
                                      ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountNeqCParFalseNodeTest, Construction) { construction(); }

TEST_F(CountNeqCParFalseNodeTest, Application) { application(); }

TEST_F(CountNeqCParFalseNodeTest, Propagation) { propagation(); }

class CountNeqCParTrueNodeTest
    : public AbstractCountNeqNodeTest<true, false,
                                      ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountNeqCParTrueNodeTest, Construction) { construction(); }

TEST_F(CountNeqCParTrueNodeTest, Application) { application(); }

TEST_F(CountNeqCParTrueNodeTest, Propagation) { propagation(); }

class CountNeqYParCParNodeTest
    : public AbstractCountNeqNodeTest<true, false, ConstraintType::NORMAL> {};

TEST_F(CountNeqYParCParNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParCParNodeTest, Application) { application(); }

TEST_F(CountNeqYParCParNodeTest, Propagation) { propagation(); }

class CountNeqYParCParReifNodeTest
    : public AbstractCountNeqNodeTest<true, false, ConstraintType::REIFIED> {};

TEST_F(CountNeqYParCParReifNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParCParReifNodeTest, Application) { application(); }

TEST_F(CountNeqYParCParReifNodeTest, Propagation) { propagation(); }

class CountNeqYParCParFalseNodeTest
    : public AbstractCountNeqNodeTest<true, false,
                                      ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountNeqYParCParFalseNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParCParFalseNodeTest, Application) { application(); }

TEST_F(CountNeqYParCParFalseNodeTest, Propagation) { propagation(); }

class CountNeqYParCParTrueNodeTest
    : public AbstractCountNeqNodeTest<true, false,
                                      ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountNeqYParCParTrueNodeTest, Construction) { construction(); }

TEST_F(CountNeqYParCParTrueNodeTest, Application) { application(); }

TEST_F(CountNeqYParCParTrueNodeTest, Propagation) { propagation(); }