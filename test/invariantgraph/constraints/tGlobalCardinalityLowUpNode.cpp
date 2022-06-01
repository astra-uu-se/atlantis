#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/globalCardinalityLowUpNode.hpp"

static bool isSatisfied(const std::vector<Int>& values,
                        const std::vector<Int>& cover,
                        const std::vector<Int>& low,
                        const std::vector<Int>& up) {
  std::vector<Int> counts(cover.size(), 0);
  for (const Int val : values) {
    for (size_t i = 0; i < cover.size(); ++i) {
      if (val == cover.at(i)) {
        counts.at(i)++;
        break;
      }
    }
  }
  bool sat = true;
  for (size_t i = 0; i < counts.size(); ++i) {
    sat = sat && low.at(i) <= counts.at(i) && counts.at(i) <= up.at(i);
  }
  return sat;
}

template <ConstraintType Type>
class AbstractGlobalCardinalityLowUpNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(x1, 5, 10);
  INT_VARIABLE(x2, 2, 7);
  const std::vector<Int> cover{2, 6};
  const std::vector<Int> low{0, 1};
  const std::vector<Int> up{1, 2};
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;
  std::unique_ptr<invariantgraph::GlobalCardinalityLowUpNode> node;

  void SetUp() override {
    if constexpr (Type == ConstraintType::REIFIED) {
      fznparser::Constraint cnstr{
          "fzn_global_cardinality_low_up_reif",
          {fznparser::Constraint::ArrayArgument{"x1", "x2"},
           fznparser::Constraint::ArrayArgument{cover.at(0), cover.at(1)},
           fznparser::Constraint::ArrayArgument{low.at(0), low.at(1)},
           fznparser::Constraint::ArrayArgument{up.at(0), up.at(1)},
           fznparser::Constraint::Argument{"r"}},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {x1, x2, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        fznparser::Constraint cnstr{
            "fzn_global_cardinality_low_up",
            {fznparser::Constraint::ArrayArgument{"x1", "x2"},
             fznparser::Constraint::ArrayArgument{cover.at(0), cover.at(1)},
             fznparser::Constraint::ArrayArgument{low.at(0), low.at(1)},
             fznparser::Constraint::ArrayArgument{up.at(0), up.at(1)}},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        fznparser::Constraint cnstr{
            "fzn_global_cardinality_low_up_reif",
            {fznparser::Constraint::ArrayArgument{"x1", "x2"},
             fznparser::Constraint::ArrayArgument{cover.at(0), cover.at(1)},
             fznparser::Constraint::ArrayArgument{low.at(0), low.at(1)},
             fznparser::Constraint::ArrayArgument{up.at(0), up.at(1)}, false},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else {
        fznparser::Constraint cnstr{
            "fzn_global_cardinality_low_up_reif",
            {fznparser::Constraint::ArrayArgument{"x1", "x2"},
             fznparser::Constraint::ArrayArgument{cover.at(0), cover.at(1)},
             fznparser::Constraint::ArrayArgument{low.at(0), low.at(1)},
             fznparser::Constraint::ArrayArgument{up.at(0), up.at(1)}, true},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      }
      fznparser::FZNModel mdl{
          {}, {x1, x2}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }
    setModel(model.get());

    node = makeNode<invariantgraph::GlobalCardinalityLowUpNode>(*constraint);
  }

  void construction() {
    const size_t numInputs = 2;
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
      EXPECT_EQ(node->definedVariables().size(), 0);
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
    registerVariables(engine, {x1.name, x2.name});

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

    // x1, x2
    EXPECT_EQ(engine.searchVariables().size(), 2);
    // x1, x2, violation
    // violation
    EXPECT_EQ(engine.numVariables(), 3);
    // gcc
    EXPECT_EQ(engine.numInvariants(), 1);
    EXPECT_EQ(engine.lowerBound(node->violationVarId()), 0);
    EXPECT_GT(engine.upperBound(node->violationVarId()), 0);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {x1.name, x2.name});
    node->createDefinedVariables(engine);
    node->registerWithEngine(engine);

    std::vector<VarId> inputs;
    for (auto* const input : node->staticInputs()) {
      EXPECT_NE(input->varId(), NULL_ID);
      inputs.emplace_back(input->varId());
    }
    EXPECT_EQ(inputs.size(), 2);

    EXPECT_NE(node->violationVarId(), NULL_ID);
    const VarId violationId = node->violationVarId();

    std::vector<Int> inputVals(inputs.size());

    engine.close();

    for (inputVals.at(0) = engine.lowerBound(inputs.at(0));
         inputVals.at(0) <= engine.upperBound(inputs.at(0));
         ++inputVals.at(0)) {
      for (inputVals.at(1) = engine.lowerBound(inputs.at(1));
           inputVals.at(1) <= engine.upperBound(inputs.at(1));
           ++inputVals.at(1)) {
        engine.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          engine.setValue(inputs.at(i), inputVals.at(i));
        }
        engine.endMove();

        engine.beginProbe();
        engine.query(violationId);
        engine.endProbe();

        if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
          bool sat = isSatisfied(inputVals, cover, low, up);
          EXPECT_EQ(engine.currentValue(violationId) == 0, sat);
        } else {
          bool sat = isSatisfied(inputVals, cover, low, up);
          EXPECT_NE(engine.currentValue(violationId) == 0, sat);
        }
      }
    }
  }
};

class GlobalCardinalityLowUpNodeTest
    : public AbstractGlobalCardinalityLowUpNodeTest<ConstraintType::NORMAL> {};

TEST_F(GlobalCardinalityLowUpNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityLowUpNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityLowUpNodeTest, Propagation) { propagation(); }

class GlobalCardinalityLowUpReifNodeTest
    : public AbstractGlobalCardinalityLowUpNodeTest<ConstraintType::REIFIED> {};

TEST_F(GlobalCardinalityLowUpReifNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityLowUpReifNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityLowUpReifNodeTest, Propagation) { propagation(); }

class GlobalCardinalityLowUpFalseNodeTest
    : public AbstractGlobalCardinalityLowUpNodeTest<
          ConstraintType::CONSTANT_FALSE> {};

TEST_F(GlobalCardinalityLowUpFalseNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityLowUpFalseNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityLowUpFalseNodeTest, Propagation) { propagation(); }

class GlobalCardinalityLowUpTrueNodeTest
    : public AbstractGlobalCardinalityLowUpNodeTest<
          ConstraintType::CONSTANT_TRUE> {};

TEST_F(GlobalCardinalityLowUpTrueNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityLowUpTrueNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityLowUpTrueNodeTest, Propagation) { propagation(); }