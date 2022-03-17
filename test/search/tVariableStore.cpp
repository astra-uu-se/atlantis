#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "search/variableStore.hpp"

class VariableStoreTest : public testing::Test {
 public:
  PropagationEngine engine;
  search::VariableStore::VariableMap variables;

  VarId variableInModel;
  VarId variableNotInModel;

  void SetUp() override {
    auto fznVariable = std::make_shared<fznparser::SearchVariable>(
        "a", fznparser::AnnotationCollection(),
        std::make_unique<fznparser::IntDomain>(5, 10));

    engine.open();
    variableNotInModel = engine.makeIntVar(30, 30, 40);

    variableInModel = engine.makeIntVar(0, 0, 20);
    variables.emplace(variableInModel, fznVariable);
    engine.close();
  }
};

TEST_F(VariableStoreTest, domains_are_constructed_from_the_engine) {
  search::VariableStore store(engine, variables);

  auto domain = store.domain(variableNotInModel);
  EXPECT_EQ(domain.lowerBound, 30);
  EXPECT_EQ(domain.upperBound, 40);
}

TEST_F(VariableStoreTest,
       domains_from_model_variables_are_not_from_the_engine) {
  search::VariableStore store(engine, variables);

  auto domain = store.domain(variableInModel);
  EXPECT_EQ(domain.lowerBound, 5);
  EXPECT_EQ(domain.upperBound, 10);
}
