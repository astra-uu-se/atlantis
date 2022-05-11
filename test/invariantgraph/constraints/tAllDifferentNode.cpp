#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/allDifferentNode.hpp"

static bool isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if (values.at(i) == values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

template <bool IsReified>
class AbstractAllDifferentNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  INT_VARIABLE(c, 2, 7);
  INT_VARIABLE(d, 2, 7);
  BOOL_VARIABLE(r);

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::FZNModel> model;
  std::unique_ptr<invariantgraph::AllDifferentNode> node;

  void SetUp() override {
    if constexpr (!IsReified) {
      fznparser::Constraint cnstr{
          "alldifferent",
          {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {a, b, c, d}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    } else {
      fznparser::Constraint cnstr{
          "alldifferent_reif",
          {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"},
           fznparser::Constraint::Argument{"r"}},
          {}};

      constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

      fznparser::FZNModel mdl{
          {}, {a, b, c, d, r}, {*constraint}, fznparser::Satisfy{}};

      model = std::make_unique<fznparser::FZNModel>(std::move(mdl));
    }

    setModel(model.get());
    node = makeNode<invariantgraph::AllDifferentNode>(*constraint);
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
    if constexpr (!IsReified) {
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
    EXPECT_EQ(engine.upperBound(node->violationVarId()), 3);
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

            EXPECT_EQ(engine.currentValue(violationId) > 0,
                      isViolating(values));
          }
        }
      }
    }
  }
};

class AllDifferentNodeTest : public AbstractAllDifferentNodeTest<false> {};

TEST_F(AllDifferentNodeTest, Construction) { construction(); }

TEST_F(AllDifferentNodeTest, Application) { application(); }

TEST_F(AllDifferentNodeTest, Propagation) { propagation(); }

class AllDifferentReifNodeTest : public AbstractAllDifferentNodeTest<true> {};

TEST_F(AllDifferentReifNodeTest, Construction) { construction(); }

TEST_F(AllDifferentReifNodeTest, Application) { application(); }

TEST_F(AllDifferentReifNodeTest, Propagation) { propagation(); }