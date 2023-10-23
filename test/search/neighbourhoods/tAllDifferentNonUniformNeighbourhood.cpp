#include <gtest/gtest.h>

#include <unordered_set>

#include "search/annealing/annealerContainer.hpp"
#include "search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class AlwaysAcceptingAnnealer : public search::Annealer {
 public:
  AlwaysAcceptingAnnealer(const search::Assignment& assignment,
                          search::RandomProvider& random,
                          search::AnnealingSchedule& schedule)
      : Annealer(assignment, random, schedule) {}
  virtual ~AlwaysAcceptingAnnealer() = default;

 protected:
  [[nodiscard]] bool accept(Int) override { return true; }
};

class AllDifferentNonUniformNeighbourhoodTest : public ::testing::Test {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::unique_ptr<search::Assignment> assignment;
  search::RandomProvider random{123456789};

  std::vector<search::SearchVariable> variables;
  std::vector<std::vector<Int>> domains{
      std::vector<Int>{1, 3, 4},
      std::vector<Int>{1, 4},
      std::vector<Int>{2, 4, 5},
  };

  Int domainLb{
      *std::min_element(domains.front().begin(), domains.front().end())};
  Int domainUb{
      *std::max_element(domains.front().begin(), domains.front().end())};

  void SetUp() override {
    solver = std::make_unique<propagation::Solver>();
    solver->open();
    assignment = std::make_unique<search::Assignment>(
        *solver, solver->makeIntVar(0, 0, 0), solver->makeIntVar(0, 0, 0),
        propagation::ObjectiveDirection::NONE);
    for (auto i = 0u; i < domains.size(); ++i) {
      const auto& [lb, ub] =
          std::minmax_element(domains.at(i).begin(), domains.at(i).end());

      propagation::VarId var = solver->makeIntVar(*lb, *lb, *ub);
      domainLb = std::min(domainLb, *lb);
      domainUb = std::max(domainUb, *ub);
      variables.emplace_back(var, SearchDomain(domains.at(i)));
    }
    solver->close();
  }
};

TEST_F(AllDifferentNonUniformNeighbourhoodTest, Initialize) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVariable>(variables), domainLb, domainUb,
      *solver);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < variables.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }

  std::unordered_set<Int> usedValues{};
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });
    usedValues.clear();
    for (auto i = 0u; i < variables.size(); ++i) {
      const Int value = solver->committedValue(variables.at(i).solverId());
      EXPECT_FALSE(usedValues.contains(value));
      usedValues.emplace(value);
      EXPECT_TRUE(setDomains.at(i).contains(value));
    }
    EXPECT_EQ(usedValues.size(), variables.size());
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, CanSwap) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::move(std::vector<search::SearchVariable>(variables)), domainLb,
      domainUb, *solver);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < variables.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

    for (size_t variable1Index = 0; variable1Index < variables.size();
         ++variable1Index) {
      const Int value1 =
          solver->committedValue(variables.at(variable1Index).solverId());
      for (size_t variable2Index = 0; variable2Index < variables.size();
           ++variable2Index) {
        if (variable1Index == variable2Index) {
          continue;
        }
        const Int value2 =
            solver->committedValue(variables.at(variable2Index).solverId());
        EXPECT_GE(value2, domainLb);
        if (!setDomains.at(variable1Index).contains(value2)) {
          continue;
        }
        const size_t value2Index = static_cast<size_t>(value2 - domainLb);
        bool expected = setDomains.at(variable2Index).contains(value1);
        bool actual =
            neighbourhood.canSwap(*assignment, variable1Index, value2Index);
        EXPECT_EQ(expected, actual);
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, Swap) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::move(std::vector<search::SearchVariable>(variables)), domainLb,
      domainUb, *solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*assignment, random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < variables.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

    for (size_t variable1Index = 0; variable1Index < variables.size();
         ++variable1Index) {
      const Int value1 =
          solver->committedValue(variables.at(variable1Index).solverId());
      for (size_t variable2Index = 0; variable2Index < variables.size();
           ++variable2Index) {
        if (variable1Index == variable2Index) {
          continue;
        }
        const Int value2 =
            solver->committedValue(variables.at(variable2Index).solverId());
        EXPECT_GE(value2, domainLb);
        if (!setDomains.at(variable1Index).contains(value2)) {
          continue;
        }
        const size_t value2Index = static_cast<size_t>(value2 - domainLb);
        if (setDomains.at(variable2Index).contains(value1)) {
          neighbourhood.swapValues(*assignment, annealer, variable1Index,
                                   value2Index);
        }
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, AssignValue) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::move(std::vector<search::SearchVariable>(variables)), domainLb,
      domainUb, *solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*assignment, random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < variables.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

    for (size_t variableIndex = 0; variableIndex < variables.size();
         ++variableIndex) {
      for (const Int newValue : domains.at(variableIndex)) {
        const Int oldValue =
            solver->committedValue(variables.at(variableIndex).solverId());
        EXPECT_GE(newValue, domainLb);
        bool freeValue = true;
        for (const auto& variable : variables) {
          if (newValue == solver->committedValue(variable.solverId())) {
            freeValue = false;
            break;
          }
        }
        if (!freeValue) {
          continue;
        }
        const size_t newValueIndex = static_cast<size_t>(newValue - domainLb);
        EXPECT_EQ(oldValue, solver->committedValue(
                                variables.at(variableIndex).solverId()));
        neighbourhood.assignValue(*assignment, annealer, variableIndex,
                                  newValueIndex);
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, RandomMove) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::move(std::vector<search::SearchVariable>(variables)), domainLb,
      domainUb, *solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*assignment, random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < variables.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

    EXPECT_TRUE(neighbourhood.randomMove(random, *assignment, annealer));
  }
}
}  // namespace atlantis::testing