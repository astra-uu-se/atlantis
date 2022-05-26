#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/allEqualNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if (values.at(i) != values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

template <ConstraintType Type>
class AbstractAllEqualNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  INT_VARIABLE(c, 2, 7);
  INT_VARIABLE(d, 2, 7);
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;
  std::unique_ptr<invariantgraph::AllEqualNode> node;

  void SetUp() override {
    if constexpr (Type == ConstraintType::REIFIED) {
      fznparser::Constraint cnstr{
          "fzn_all_equal_int_reif",
          {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"},
           fznparser::Constraint::Argument{"r"}},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {a, b, c, d, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        fznparser::Constraint cnstr{
            "fzn_all_equal_int",
            {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        fznparser::Constraint cnstr{
            "fzn_all_equal_int_reif",
            {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}, false},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      } else {
        fznparser::Constraint cnstr{
            "fzn_all_equal_int_reif",
            {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}, true},
            {}};
        constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));
      }

      fznparser::FZNModel mdl{
          {}, {a, b, c, d}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::AllEqualNode>(*constraint);
  }

  void construction() {
    EXPECT_EQ(node->staticInputs().size(), 4);
    std::vector<invariantgraph::VariableNode*> expectedVars;
    for (size_t i = 0; i < 4; ++i) {
      expectedVars.emplace_back(_variables.at(i).get());
    }
    EXPECT_EQ(node->staticInputs(), expectedVars);
    EXPECT_THAT(expectedVars, testing::ContainerEq(node->staticInputs()));
    expectMarkedAsInput(node.get(), node->staticInputs());
    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(node->isReified());
      EXPECT_EQ(node->reifiedViolation(), nullptr);
    } else {
      EXPECT_TRUE(node->isReified());
      EXPECT_NE(node->reifiedViolation(), nullptr);
      EXPECT_EQ(node->reifiedViolation()->variable(),
                invariantgraph::VariableNode::FZNVariable(r));
    }
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {a.name, b.name, c.name, d.name});
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

    // a, b, c and d
    EXPECT_EQ(engine.searchVariables().size(), 4);

    // a, b, c, d and the violation
    EXPECT_EQ(engine.numVariables(), 5);

    // alldifferent
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(node->violationVarId()), 0);
    EXPECT_GT(engine.upperBound(node->violationVarId()), 0);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {a.name, b.name, c.name, d.name});
    node->createDefinedVariables(engine);
    node->registerWithEngine(engine);

    std::vector<VarId> inputs;
    EXPECT_EQ(node->staticInputs().size(), 4);
    for (auto* const inputVariable : node->staticInputs()) {
      EXPECT_NE(inputVariable->varId(), NULL_ID);
      inputs.emplace_back(inputVariable->varId());
    }

    EXPECT_NE(node->violationVarId(), NULL_ID);
    const VarId violationId = node->violationVarId();
    EXPECT_EQ(inputs.size(), 4);

    std::vector<Int> values(inputs.size());
    engine.close();

    for (values.at(0) = engine.lowerBound(inputs.at(0));
         values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = engine.lowerBound(inputs.at(1));
           values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = engine.lowerBound(inputs.at(2));
             values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
          for (values.at(3) = engine.lowerBound(inputs.at(3));
               values.at(3) <= engine.upperBound(inputs.at(3));
               ++values.at(3)) {
            engine.beginMove();
            for (size_t i = 0; i < inputs.size(); ++i) {
              engine.setValue(inputs.at(i), values.at(i));
            }
            engine.endMove();

            engine.beginProbe();
            engine.query(violationId);
            engine.endProbe();

            if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
              EXPECT_EQ(engine.currentValue(violationId) > 0,
                        isViolating(values));
            } else {
              EXPECT_NE(engine.currentValue(violationId) > 0,
                        isViolating(values));
            }
          }
        }
      }
    }
  }
};

class AllEqualNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::NORMAL> {};

TEST_F(AllEqualNodeTest, Construction) { construction(); }

TEST_F(AllEqualNodeTest, Application) { application(); }

TEST_F(AllEqualNodeTest, Propagation) { propagation(); }

class AllEqualReifNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::REIFIED> {};

TEST_F(AllEqualReifNodeTest, Construction) { construction(); }

TEST_F(AllEqualReifNodeTest, Application) { application(); }

TEST_F(AllEqualReifNodeTest, Propagation) { propagation(); }

class AllEqualFalseNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(AllEqualFalseNodeTest, Construction) { construction(); }

TEST_F(AllEqualFalseNodeTest, Application) { application(); }

TEST_F(AllEqualFalseNodeTest, Propagation) { propagation(); }

class AllEqualTrueNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(AllEqualTrueNodeTest, Construction) { construction(); }

TEST_F(AllEqualTrueNodeTest, Application) { application(); }

TEST_F(AllEqualTrueNodeTest, Propagation) { propagation(); }