#include <gtest/gtest.h>

#include "search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class AllDifferentUniformNeighbourhoodTest : public ::testing::Test {
 public:
  propagation::PropagationEngine engine;
  search::Assignment assignment{engine, propagation::NULL_ID,
                                propagation::NULL_ID,
                                propagation::ObjectiveDirection::NONE};
  search::RandomProvider random{123456789};

  std::vector<search::SearchVariable> variables;
  std::vector<Int> domain{1, 2, 3, 4};

  void SetUp() override {
    engine.open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarId var = engine.makeIntVar(1, 1, 4);
      variables.emplace_back(var, SearchDomain(1, 4));
    }
    engine.close();
  }
};

TEST_F(AllDifferentUniformNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::AllDifferentUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVariable>(variables), domain, engine);

  assignment.assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  for (const auto& var : variables) {
    EXPECT_TRUE(engine.committedValue(var.engineId()) >= 1);
    EXPECT_TRUE(engine.committedValue(var.engineId()) <= 4);
  }
}

}  // namespace atlantis::testing