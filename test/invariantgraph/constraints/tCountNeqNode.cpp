#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/countNeqNode.hpp"

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
  std::unique_ptr<invariantgraph::CountNeqNode> node;

  void SetUp() override {
    if constexpr (Type == ConstraintType::REIFIED) {
      if constexpr (YIsParameter) {
        if constexpr (CIsParameter) {
          fznparser::Constraint cnstr{
              "fzn_count_neq_reif",
              {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
               yParamVal, cParamVal, fznparser::Constraint::Argument{"r"}},
              {}};

          constraint =
              std::make_unique<fznparser::Constraint>(std::move(cnstr));

          fznparser::FZNModel mdl{
              {}, {x1, x2, x3, r}, {*constraint}, fznparser::Satisfy{}};

          model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
        } else {
          fznparser::Constraint cnstr{
              "fzn_count_neq_reif",
              {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
               yParamVal, fznparser::Constraint::Argument{"c"},
               fznparser::Constraint::Argument{"r"}},
              {}};

          constraint =
              std::make_unique<fznparser::Constraint>(std::move(cnstr));

          fznparser::FZNModel mdl{
              {}, {x1, x2, x3, c, r}, {*constraint}, fznparser::Satisfy{}};

          model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
        }
      } else {
        if constexpr (CIsParameter) {
          fznparser::Constraint cnstr{
              "fzn_count_neq_reif",
              {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
               fznparser::Constraint::Argument{"y"}, cParamVal,
               fznparser::Constraint::Argument{"r"}},
              {}};

          constraint =
              std::make_unique<fznparser::Constraint>(std::move(cnstr));

          fznparser::FZNModel mdl{
              {}, {x1, x2, x3, y, r}, {*constraint}, fznparser::Satisfy{}};

          model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
        } else {
          fznparser::Constraint cnstr{
              "fzn_count_neq_reif",
              {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
               fznparser::Constraint::Argument{"y"},
               fznparser::Constraint::Argument{"c"},
               fznparser::Constraint::Argument{"r"}},
              {}};

          constraint =
              std::make_unique<fznparser::Constraint>(std::move(cnstr));

          fznparser::FZNModel mdl{
              {}, {x1, x2, x3, y, c, r}, {*constraint}, fznparser::Satisfy{}};

          model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
        }
      }
    } else {
      // No variable reification:
      if constexpr (Type == ConstraintType::NORMAL) {
        if constexpr (YIsParameter) {
          if constexpr (CIsParameter) {
            fznparser::Constraint cnstr{
                "fzn_count_neq",
                {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                 yParamVal, cParamVal},
                {}};
            constraint =
                std::make_unique<fznparser::Constraint>(std::move(cnstr));
            fznparser::FZNModel mdl{
                {}, {x1, x2, x3}, {*constraint}, fznparser::Satisfy{}};

            model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
          } else {
            fznparser::Constraint cnstr{
                "fzn_count_neq",
                {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                 yParamVal, fznparser::Constraint::Argument{"c"}},
                {}};
            constraint =
                std::make_unique<fznparser::Constraint>(std::move(cnstr));
            fznparser::FZNModel mdl{
                {}, {x1, x2, x3, c}, {*constraint}, fznparser::Satisfy{}};

            model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
          }
        } else {
          if constexpr (CIsParameter) {
            fznparser::Constraint cnstr{
                "fzn_count_neq",
                {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                 fznparser::Constraint::Argument{"y"}, cParamVal},
                {}};
            constraint =
                std::make_unique<fznparser::Constraint>(std::move(cnstr));
            fznparser::FZNModel mdl{
                {}, {x1, x2, x3, y}, {*constraint}, fznparser::Satisfy{}};

            model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
          } else {
            fznparser::Constraint cnstr{
                "fzn_count_neq",
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
        }
      } else {
        // constant reification:
        if constexpr (YIsParameter) {
          if constexpr (CIsParameter) {
            if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
              fznparser::Constraint cnstr{
                  "fzn_count_neq_reif",
                  {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                   yParamVal, cParamVal, false},
                  {}};
              constraint =
                  std::make_unique<fznparser::Constraint>(std::move(cnstr));
            } else {
              fznparser::Constraint cnstr{
                  "fzn_count_neq_reif",
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
            // C is var
            if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
              fznparser::Constraint cnstr{
                  "fzn_count_neq_reif",
                  {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                   yParamVal, fznparser::Constraint::Argument{"c"}, false},
                  {}};
              constraint =
                  std::make_unique<fznparser::Constraint>(std::move(cnstr));
            } else {
              fznparser::Constraint cnstr{
                  "fzn_count_neq_reif",
                  {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                   yParamVal, fznparser::Constraint::Argument{"c"}, true},
                  {}};
              constraint =
                  std::make_unique<fznparser::Constraint>(std::move(cnstr));
            }
            fznparser::FZNModel mdl{
                {}, {x1, x2, x3, c}, {*constraint}, fznparser::Satisfy{}};
            model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
          }
        } else {
          // y is var
          if constexpr (CIsParameter) {
            if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
              fznparser::Constraint cnstr{
                  "fzn_count_neq_reif",
                  {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                   fznparser::Constraint::Argument{"y"}, cParamVal, false},
                  {}};
              constraint =
                  std::make_unique<fznparser::Constraint>(std::move(cnstr));
            } else {
              fznparser::Constraint cnstr{
                  "fzn_count_neq_reif",
                  {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                   fznparser::Constraint::Argument{"y"}, cParamVal, true},
                  {}};
              constraint =
                  std::make_unique<fznparser::Constraint>(std::move(cnstr));
            }
            fznparser::FZNModel mdl{
                {}, {x1, x2, x3, y}, {*constraint}, fznparser::Satisfy{}};
            model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
          } else {
            // c is var
            if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
              fznparser::Constraint cnstr{
                  "fzn_count_neq_reif",
                  {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                   fznparser::Constraint::Argument{"y"},
                   fznparser::Constraint::Argument{"c"}, false},
                  {}};
              constraint =
                  std::make_unique<fznparser::Constraint>(std::move(cnstr));
            } else {
              fznparser::Constraint cnstr{
                  "fzn_count_neq_reif",
                  {fznparser::Constraint::ArrayArgument{"x1", "x2", "x3"},
                   fznparser::Constraint::Argument{"y"},
                   fznparser::Constraint::Argument{"c"}, true},
                  {}};
              constraint =
                  std::make_unique<fznparser::Constraint>(std::move(cnstr));
            }
            fznparser::FZNModel mdl{
                {}, {x1, x2, x3, y, c}, {*constraint}, fznparser::Satisfy{}};
            model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
          }
        }
      }
    }

    setModel(model.get());

    node = makeNode<invariantgraph::CountNeqNode>(*constraint);
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
      if constexpr (CIsParameter) {
        registerVariables(engine, {x1.name, x2.name, x3.name});
      } else {
        registerVariables(engine, {x1.name, x2.name, x3.name, c.name});
      }
    } else if constexpr (CIsParameter) {
      registerVariables(engine, {x1.name, x2.name, x3.name, y.name});
    } else {
      registerVariables(engine, {x1.name, x2.name, x3.name, y.name, c.name});
    }
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_EQ(definedVariable->varId(), NULL_ID);
    }
    EXPECT_EQ(node->violationVarId(), NULL_ID);
    node->createDefinedVariables(engine);
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_NE(definedVariable->varId(), NULL_ID);
    }
    EXPECT_NE(node->violationVarId(), NULL_ID);
    node->registerWithEngine(engine);
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
    if constexpr (YIsParameter) {
      if constexpr (CIsParameter) {
        registerVariables(engine, {x1.name, x2.name, x3.name});
      } else {
        registerVariables(engine, {x1.name, x2.name, x3.name, c.name});
      }
    } else if constexpr (CIsParameter) {
      registerVariables(engine, {x1.name, x2.name, x3.name, y.name});
    } else {
      registerVariables(engine, {x1.name, x2.name, x3.name, y.name, c.name});
    }
    node->createDefinedVariables(engine);
    node->registerWithEngine(engine);

    std::vector<VarId> inputs;
    size_t numStaticInputs = 5;
    if constexpr (YIsParameter) {
      --numStaticInputs;
    }
    if constexpr (CIsParameter) {
      --numStaticInputs;
    }
    EXPECT_EQ(node->staticInputs().size(), numStaticInputs);

    for (auto* const inputVariable : node->staticInputs()) {
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