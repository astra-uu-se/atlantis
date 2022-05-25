#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/countEqNode.hpp"

static Int computeOutput(const Int y, const std::vector<Int>& values) {
  Int count = 0;
  for (const Int val : values) {
    count += static_cast<Int>(val == y);
  }
  return count;
}

static bool isViolating(const Int y, const std::vector<Int>& values,
                        const Int c) {
  return computeOutput(y, values) != c;
}

template <bool YIsParameter, ConstraintType Type>
class AbstractCountEqNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(x1, 5, 10);
  INT_VARIABLE(x2, 2, 7);
  INT_VARIABLE(x3, 2, 7);
  const Int yParamVal{5};
  INT_VARIABLE(y, 2, 7);
  BOOL_VARIABLE(r);
  INT_VARIABLE(c, 2, 7);
  const Int cParamVal{2};

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;
  std::unique_ptr<invariantgraph::CountEqNode> node;

  void SetUp() override {
    if constexpr (Type == ConstraintType::REIFIED) {
      if constexpr (YIsParameter) {
        fznparser::Constraint cnstr{
            "count_eq_reif",
            {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"}, yParamVal,
             cParamVal, fznparser::Constraint::Argument{"r"}},
            {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

        fznparser::FZNModel mdl{
            {}, {x1, x2, x3, r}, {*constraint}, fznparser::Satisfy{}};

        model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
      } else {
        fznparser::Constraint cnstr{
            "count_eq_reif",
            {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
             fznparser::Constraint::Argument{"y"}, cParamVal,
             fznparser::Constraint::Argument{"r"}},
            {}};

        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

        fznparser::FZNModel mdl{
            {}, {x1, x2, x3, y, r}, {*constraint}, fznparser::Satisfy{}};

        model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
      }
    } else {
      // No variable reification:
      if constexpr (Type == ConstraintType::NORMAL) {
        if constexpr (YIsParameter) {
          fznparser::Constraint cnstr{
              "count_eq",
              {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
               yParamVal, fznparser::Constraint::Argument{"c"}},
              {}};
          constraint =
              std::make_unique<fznparser::Constraint>(std::move(cnstr));
          fznparser::FZNModel mdl{
              {}, {x1, x2, x3, c}, {*constraint}, fznparser::Satisfy{}};

          model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
        } else {
          fznparser::Constraint cnstr{
              "count_eq",
              {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
               fznparser::Constraint::Argument{"y"},
               fznparser::Constraint::Argument{"c"}},
              {}};
          constraint =
              std::make_unique<fznparser::Constraint>(std::move(cnstr));
          fznparser::FZNModel mdl{
              {}, {x1, x2, x3, y, c}, {*constraint}, fznparser::Satisfy{}};

          model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
        }
      } else {
        // constant reification:
        if constexpr (YIsParameter) {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            fznparser::Constraint cnstr{
                "count_eq_reif",
                {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                 yParamVal, cParamVal, false},
                {}};
            constraint =
                std::make_unique<fznparser::Constraint>(std::move(cnstr));
          } else {
            fznparser::Constraint cnstr{
                "count_eq_reif",
                {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                 yParamVal, cParamVal, true},
                {}};
            constraint =
                std::make_unique<fznparser::Constraint>(std::move(cnstr));
          }
          fznparser::FZNModel mdl{
              {}, {x1, x2, x3}, {*constraint}, fznparser::Satisfy{}};
          model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
        } else {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            fznparser::Constraint cnstr{
                "count_eq_reif",
                {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                 fznparser::Constraint::Argument{"y"}, cParamVal, false},
                {}};
            constraint =
                std::make_unique<fznparser::Constraint>(std::move(cnstr));
          } else {
            fznparser::Constraint cnstr{
                "count_eq_reif",
                {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                 fznparser::Constraint::Argument{"y"}, cParamVal, true},
                {}};
            constraint =
                std::make_unique<fznparser::Constraint>(std::move(cnstr));
          }
          fznparser::FZNModel mdl{
              {}, {x1, x2, x3, y}, {*constraint}, fznparser::Satisfy{}};
          model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
        }
      }
    }

    setModel(model.get());

    node = makeNode<invariantgraph::CountEqNode>(*constraint);
  }

  void construction() {
    size_t inputSize = 4;
    if constexpr (YIsParameter) {
      inputSize = 3;
    }
    EXPECT_EQ(node->staticInputs().size(), inputSize);
    std::vector<invariantgraph::VariableNode*> expectedInputs;
    for (size_t i = 0; i < inputSize; ++i) {
      expectedInputs.emplace_back(_variables.at(i).get());
    }
    EXPECT_EQ(node->staticInputs(), expectedInputs);
    EXPECT_THAT(expectedInputs, testing::ContainerEq(node->staticInputs()));
    expectMarkedAsInput(node.get(), node->staticInputs());

    std::vector<invariantgraph::VariableNode*> expectedOutputs;
    for (size_t i = inputSize; i < _variables.size(); ++i) {
      expectedOutputs.emplace_back(_variables.at(i).get());
    }
    EXPECT_EQ(node->definedVariables(), expectedOutputs);
    EXPECT_THAT(expectedOutputs,
                testing::ContainerEq(node->definedVariables()));

    if constexpr (Type == ConstraintType::REIFIED) {
      EXPECT_TRUE(node->isReified());
      EXPECT_NE(node->reifiedViolation(), nullptr);
      EXPECT_EQ(node->reifiedViolation()->variable(),
                invariantgraph::VariableNode::FZNVariable(r));
    } else {
      EXPECT_FALSE(node->isReified());
      EXPECT_EQ(node->reifiedViolation(), nullptr);
    }
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    if constexpr (YIsParameter) {
      registerVariables(engine, {x1.name, x2.name, x3.name});
    } else {
      registerVariables(engine, {x1.name, x2.name, x3.name, y.name});
    }
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_EQ(definedVariable->varId(), NULL_ID);
    }
    EXPECT_EQ(node->violationVarId(), NULL_ID);
    node->createDefinedVariables(engine);
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_NE(definedVariable->varId(), NULL_ID);
    }
    if constexpr (Type == ConstraintType::NORMAL) {
      EXPECT_EQ(node->violationVarId(), NULL_ID);
    } else {
      EXPECT_NE(node->violationVarId(), NULL_ID);
    }
    node->registerWithEngine(engine);
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

    for (auto* definedVariable : node->definedVariables()) {
      EXPECT_EQ(engine.lowerBound(definedVariable->varId()), 0);
      EXPECT_GT(engine.upperBound(definedVariable->varId()), 0);
    }
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    if constexpr (YIsParameter) {
      registerVariables(engine, {x1.name, x2.name, x3.name});
    } else {
      registerVariables(engine, {x1.name, x2.name, x3.name, y.name});
    }
    node->createDefinedVariables(engine);
    node->registerWithEngine(engine);

    std::vector<VarId> inputs;
    if constexpr (YIsParameter) {
      EXPECT_EQ(node->staticInputs().size(), 3);
    } else {
      EXPECT_EQ(node->staticInputs().size(), 4);
    }
    for (auto* const inputVariable : node->staticInputs()) {
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
                        computeOutput(yVal, values));
            }
            if (violationId != NULL_ID) {
              if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
                EXPECT_EQ(engine.currentValue(violationId) > 0,
                          isViolating(yVal, values, cParamVal));
              } else {
                EXPECT_NE(engine.currentValue(violationId) > 0,
                          isViolating(yVal, values, cParamVal));
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

class CountEqConstYNodeTest
    : public AbstractCountEqNodeTest<true, ConstraintType::NORMAL> {};

TEST_F(CountEqConstYNodeTest, Construction) { construction(); }

TEST_F(CountEqConstYNodeTest, Application) { application(); }

TEST_F(CountEqConstYNodeTest, Propagation) { propagation(); }

class CountEqConstYReifNodeTest
    : public AbstractCountEqNodeTest<true, ConstraintType::REIFIED> {};

TEST_F(CountEqConstYReifNodeTest, Construction) { construction(); }

TEST_F(CountEqConstYReifNodeTest, Application) { application(); }

TEST_F(CountEqConstYReifNodeTest, Propagation) { propagation(); }

class CountEqConstYFalseNodeTest
    : public AbstractCountEqNodeTest<true, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountEqConstYFalseNodeTest, Construction) { construction(); }

TEST_F(CountEqConstYFalseNodeTest, Application) { application(); }

TEST_F(CountEqConstYFalseNodeTest, Propagation) { propagation(); }

class CountEqConstYTrueNodeTest
    : public AbstractCountEqNodeTest<true, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountEqConstYTrueNodeTest, Construction) { construction(); }

TEST_F(CountEqConstYTrueNodeTest, Application) { application(); }

TEST_F(CountEqConstYTrueNodeTest, Propagation) { propagation(); }