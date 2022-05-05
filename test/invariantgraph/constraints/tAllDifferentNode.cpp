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
    fznparser::Constraint cnstr{
        "alldifferent",
        {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}},
        {}};

    constraint = std::make_unique<fznparser::Constraint>(std::move(cnstr));

    fznparser::FZNModel mdl{
        {}, {a, b, c, d}, {*constraint}, fznparser::Satisfy{}};

    model = std::make_unique<fznparser::FZNModel>(std::move(mdl));

    setModel(model.get());
    node = makeNode<invariantgraph::AllDifferentNode>(*constraint);
  }

  void construction() {
    std::vector<invariantgraph::VariableNode*> expectedVars;
    std::transform(_variables.begin(), _variables.end(),
                   std::back_inserter(expectedVars),
                   [](const auto& variable) { return variable.get(); });

    EXPECT_EQ(node->staticInputs(), expectedVars);
    EXPECT_THAT(node->staticInputs(),
                testing::ContainerEq(node->staticInputs()));

    expectMarkedAsInput(node.get(), node->staticInputs());
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {a.name, b.name, c.name, d.name});
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_FALSE(_variableMap.contains(definedVariable));
    }
    EXPECT_FALSE(_variableMap.contains(node->violation()));
    node->createDefinedVariables(engine, _variableMap);
    for (auto* const definedVariable : node->definedVariables()) {
      EXPECT_TRUE(_variableMap.contains(definedVariable));
    }
    EXPECT_TRUE(_variableMap.contains(node->violation()));
    EXPECT_TRUE(_variableMap.contains(node->violation()));
    node->registerWithEngine(engine, _variableMap);
    engine.close();

    // a, b, c and d
    EXPECT_EQ(engine.searchVariables().size(), 4);

    // a, b, c, d and the violation
    EXPECT_EQ(engine.numVariables(), 5);

    // alldifferent
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(_variableMap.at(node->violation())), 0);
    EXPECT_EQ(engine.upperBound(_variableMap.at(node->violation())), 3);
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    registerVariables(engine, {a.name, b.name, c.name, d.name});
    node->createDefinedVariables(engine, _variableMap);
    node->registerWithEngine(engine, _variableMap);

    std::vector<VarId> inputs;
    EXPECT_EQ(node->staticInputs().size(), 4);
    for (auto* const inputVariable : node->staticInputs()) {
      EXPECT_TRUE(_variableMap.contains(inputVariable));
      inputs.emplace_back(_variableMap.at(inputVariable));
    }

    EXPECT_TRUE(_variableMap.contains(node->violation()));
    const VarId violationId = _variableMap.at(node->violation());
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

class AllDifferentNodeTest : public AbstractAllDifferentNodeTest<true> {};

TEST_F(AllDifferentNodeTest, Construction) { construction(); }

TEST_F(AllDifferentNodeTest, Application) { application(); }

TEST_F(AllDifferentNodeTest, Propagation) { propagation(); }

class AllDifferentReifNodeTest : public AbstractAllDifferentNodeTest<false> {};

TEST_F(AllDifferentReifNodeTest, Construction) { construction(); }

TEST_F(AllDifferentReifNodeTest, Application) { application(); }

TEST_F(AllDifferentReifNodeTest, Propagation) { propagation(); }