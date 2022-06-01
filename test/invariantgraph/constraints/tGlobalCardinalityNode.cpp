#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/globalCardinalityNode.hpp"

static std::vector<Int> computeOutputs(const std::vector<Int>& values,
                                       const std::vector<Int>& cover) {
  std::vector<Int> outputs(cover.size(), 0);
  for (size_t i = 0; i < values.size(); ++i) {
    for (size_t j = 0; j < cover.size(); ++j) {
      if (values.at(i) == cover.at(j)) {
        outputs.at(j)++;
      }
    }
  }
  return outputs;
}

static bool isSatisfied(const std::vector<Int>& values,
                        const std::vector<Int>& cover,
                        const std::vector<Int>& counts) {
  auto outputs = computeOutputs(values, cover);
  if (outputs.size() != counts.size()) {
    return false;
  }
  for (size_t i = 0; i < counts.size(); ++i) {
    if (outputs.at(i) != counts.at(i)) {
      return false;
    }
  }
  return true;
}

template <ConstraintType Type>
class AbstractGlobalCardinalityNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(x1, 5, 10);
  INT_VARIABLE(x2, 2, 7);
  const std::vector<Int> cover{2, 6};
  INT_VARIABLE(o1, 1, 2);
  INT_VARIABLE(o2, 1, 2);
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;
  std::unique_ptr<invariantgraph::GlobalCardinalityNode> node;

  void SetUp() override {
    if constexpr (Type == ConstraintType::REIFIED) {
      fznparser::Constraint cnstr{
          "fzn_global_cardinality_reif",
          {fznparser::Constraint::ArrayArgument{"x1", "x2"},
           fznparser::Constraint::ArrayArgument{cover.at(0), cover.at(1)},
           fznparser::Constraint::ArrayArgument{"o1", "o2"},
           fznparser::Constraint::Argument{"r"}},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {x1, x2, o1, o2, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        fznparser::Constraint cnstr{
            "fzn_global_cardinality",
            {fznparser::Constraint::ArrayArgument{"x1", "x2"},
             fznparser::Constraint::ArrayArgument{cover.at(0), cover.at(1)},
             fznparser::Constraint::ArrayArgument{"o1", "o2"}},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        fznparser::Constraint cnstr{
            "fzn_global_cardinality_reif",
            {fznparser::Constraint::ArrayArgument{"x1", "x2"},
             fznparser::Constraint::ArrayArgument{cover.at(0), cover.at(1)},
             fznparser::Constraint::ArrayArgument{"o1", "o2"}, false},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else {
        fznparser::Constraint cnstr{
            "fzn_global_cardinality_reif",
            {fznparser::Constraint::ArrayArgument{"x1", "x2"},
             fznparser::Constraint::ArrayArgument{cover.at(0), cover.at(1)},
             fznparser::Constraint::ArrayArgument{"o1", "o2"}, true},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      }

      fznparser::FZNModel mdl{
          {}, {x1, x2, o1, o2}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }

    setModel(model.get());

    node = makeNode<invariantgraph::GlobalCardinalityNode>(*constraint);
  }

  void construction() {
    size_t numInputs = 2;
    size_t numOutputs = 2;
    if (Type == ConstraintType::REIFIED ||
        Type == ConstraintType::CONSTANT_FALSE) {
      numInputs += 2;
      numOutputs -= 2;
    }
    EXPECT_EQ(node->staticInputs().size(), numInputs);
    std::vector<invariantgraph::VariableNode*> expectedInputs;
    for (size_t i = 0; i < numInputs; ++i) {
      expectedInputs.emplace_back(_variables.at(i).get());
    }
    EXPECT_EQ(node->staticInputs(), expectedInputs);
    EXPECT_THAT(expectedInputs, testing::ContainerEq(node->staticInputs()));
    expectMarkedAsInput(node.get(), node->staticInputs());

    if (Type == ConstraintType::REIFIED) {
      EXPECT_EQ(node->definedVariables().size(), 1);
      EXPECT_EQ(node->definedVariables().front(), _variables.back().get());
    } else {
      EXPECT_EQ(node->definedVariables().size(), numOutputs);
      std::vector<invariantgraph::VariableNode*> expectedOutputs;
      for (size_t i = numInputs; i < numInputs + numOutputs; ++i) {
        expectedOutputs.emplace_back(_variables.at(i).get());
      }
      EXPECT_EQ(node->definedVariables(), expectedOutputs);
      EXPECT_THAT(expectedOutputs,
                  testing::ContainerEq(node->definedVariables()));
    }

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
    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      registerVariables(engine, {x1.name, x2.name});
    } else {
      registerVariables(engine, {x1.name, x2.name, o1.name, o2.name});
    }

    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_EQ(definedVariable->varId(), NULL_ID);
    }
    EXPECT_EQ(node->violationVarId(), NULL_ID);
    node->createDefinedVariables(engine);
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_NE(definedVariable->varId(), NULL_ID);
    }
    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      EXPECT_EQ(node->violationVarId(), NULL_ID);
    } else {
      EXPECT_NE(node->violationVarId(), NULL_ID);
    }

    node->registerWithEngine(engine);
    engine.close();

    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      // x1, x2
      EXPECT_EQ(engine.searchVariables().size(), 2);
      // x1, x2, o1, o2
      EXPECT_EQ(engine.numVariables(), 4);
      // gcc
      EXPECT_EQ(engine.numInvariants(), 1);
    } else {
      // x1, x2, o1, o2
      EXPECT_EQ(engine.searchVariables().size(), 4);
      // x1, x2, o1, o2
      // intermediate o1, intermediate o2
      // 2 intermediate violations
      // 1 total violation
      EXPECT_EQ(engine.numVariables(), 9);
      // gcc + 2 (non)-equal + 1 total violation
      EXPECT_EQ(engine.numInvariants(), 4);

      EXPECT_EQ(engine.lowerBound(node->violationVarId()), 0);
      EXPECT_GT(engine.upperBound(node->violationVarId()), 0);
    }
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {x1.name, x2.name, o1.name, o2.name});
    node->createDefinedVariables(engine);
    node->registerWithEngine(engine);

    std::vector<VarId> inputs;
    for (auto* const input : node->inputs()) {
      EXPECT_NE(input->varId(), NULL_ID);
      inputs.emplace_back(input->varId());
    }
    EXPECT_EQ(inputs.size(), 2);

    std::vector<VarId> counts;
    for (auto* const count : node->counts()) {
      EXPECT_NE(count->varId(), NULL_ID);
      counts.emplace_back(count->varId());
    }
    EXPECT_EQ(counts.size(), 2);

    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      EXPECT_EQ(node->violationVarId(), NULL_ID);
    } else {
      EXPECT_NE(node->violationVarId(), NULL_ID);
    }
    const VarId violationId = node->violationVarId();

    std::vector<Int> inputVals(inputs.size());
    std::vector<Int> countVals(counts.size());

    std::vector<std::pair<Int, Int>> countBounds;

    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      for (const VarId c : counts) {
        countBounds.emplace_back(std::pair<Int, Int>{0, 0});
      }
    } else {
      for (const VarId c : counts) {
        countBounds.emplace_back(
            std::pair<Int, Int>{engine.lowerBound(c), engine.lowerBound(c)});
      }
    }

    engine.close();

    for (inputVals.at(0) = engine.lowerBound(inputs.at(0));
         inputVals.at(0) <= engine.upperBound(inputs.at(0));
         ++inputVals.at(0)) {
      for (inputVals.at(1) = engine.lowerBound(inputs.at(1));
           inputVals.at(1) <= engine.upperBound(inputs.at(1));
           ++inputVals.at(1)) {
        for (countVals.at(0) = countBounds.at(0).first;
             countVals.at(0) <= countBounds.at(0).second; ++countVals.at(0)) {
          for (countVals.at(1) = countBounds.at(1).first;
               countVals.at(1) <= countBounds.at(1).second; ++countVals.at(1)) {
            engine.beginMove();
            for (size_t i = 0; i < inputs.size(); ++i) {
              engine.setValue(inputs.at(i), inputVals.at(i));
            }
            if constexpr (Type == ConstraintType::CONSTANT_FALSE ||
                          Type == ConstraintType::REIFIED) {
              for (size_t i = 0; i < counts.size(); ++i) {
                engine.setValue(counts.at(i), countVals.at(i));
              }
            }
            engine.endMove();

            engine.beginProbe();
            engine.query(violationId);
            engine.endProbe();
            if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
              if constexpr (Type == ConstraintType::REIFIED) {
                EXPECT_EQ(engine.currentValue(violationId) == 0,
                          isSatisfied(inputVals, cover, countVals));
              } else {
                const std::vector<Int> actual =
                    computeOutputs(inputVals, cover);
                EXPECT_EQ(countVals.size(), counts.size());
                for (size_t i = 0; i < countVals.size(); ++i) {
                  EXPECT_EQ(engine.currentValue(counts.at(i)), actual.at(i));
                }
              }
            } else {
              EXPECT_NE(engine.currentValue(violationId) == 0,
                        isSatisfied(inputVals, cover, countVals));
            }
          }
        }
      }
    }
  }
};

class GlobalCardinalityNodeTest
    : public AbstractGlobalCardinalityNodeTest<ConstraintType::NORMAL> {};

TEST_F(GlobalCardinalityNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityNodeTest, Propagation) { propagation(); }

class GlobalCardinalityReifNodeTest
    : public AbstractGlobalCardinalityNodeTest<ConstraintType::REIFIED> {};

TEST_F(GlobalCardinalityReifNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityReifNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityReifNodeTest, Propagation) { propagation(); }

class GlobalCardinalityFalseNodeTest
    : public AbstractGlobalCardinalityNodeTest<ConstraintType::CONSTANT_FALSE> {
};

TEST_F(GlobalCardinalityFalseNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityFalseNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityFalseNodeTest, Propagation) { propagation(); }

class GlobalCardinalityTrueNodeTest
    : public AbstractGlobalCardinalityNodeTest<ConstraintType::CONSTANT_TRUE> {
};

TEST_F(GlobalCardinalityTrueNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityTrueNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityTrueNodeTest, Propagation) { propagation(); }