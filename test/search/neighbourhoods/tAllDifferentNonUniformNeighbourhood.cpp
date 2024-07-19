#include <gtest/gtest.h>

#include <unordered_set>

#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class AlwaysAcceptingAnnealer : public search::Annealer {
 public:
  AlwaysAcceptingAnnealer(const search::Assignment& assignment,
                          search::RandomProvider& random,
                          search::AnnealingSchedule& schedule)
      : Annealer(assignment, random, schedule) {}

 protected:
  [[nodiscard]] bool accept(Int) override { return true; }
};

class AllDifferentNonUniformNeighbourhoodTest : public ::testing::Test {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::unique_ptr<search::Assignment> assignment;
  search::RandomProvider random{123456789};

  std::vector<search::SearchVar> vars;
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
    for (const auto& domain : domains) {
      const auto& [lb, ub] = std::minmax_element(domain.begin(), domain.end());

      propagation::VarId var = solver->makeIntVar(*lb, *lb, *ub);
      domainLb = std::min(domainLb, *lb);
      domainUb = std::max(domainUb, *ub);
      vars.emplace_back(var, SearchDomain(domain));
    }
    solver->close();
  }
};

TEST_F(AllDifferentNonUniformNeighbourhoodTest, Initialize) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(vars), domainLb, domainUb, *solver);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < vars.size(); ++i) {
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
    for (auto i = 0u; i < vars.size(); ++i) {
      const Int value = solver->committedValue(vars.at(i).solverId());
      EXPECT_FALSE(usedValues.contains(value));
      usedValues.emplace(value);
      EXPECT_TRUE(setDomains.at(i).contains(value));
    }
    EXPECT_EQ(usedValues.size(), vars.size());
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, CanSwap) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(vars), domainLb, domainUb, *solver);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

    for (size_t var1Index = 0; var1Index < vars.size(); ++var1Index) {
      const Int value1 = solver->committedValue(vars.at(var1Index).solverId());
      for (size_t var2Index = 0; var2Index < vars.size(); ++var2Index) {
        if (var1Index == var2Index) {
          continue;
        }
        const Int value2 =
            solver->committedValue(vars.at(var2Index).solverId());
        EXPECT_GE(value2, domainLb);
        if (!setDomains.at(var1Index).contains(value2)) {
          continue;
        }
        const auto value2Index = static_cast<size_t>(value2 - domainLb);
        bool expected = setDomains.at(var2Index).contains(value1);
        bool actual =
            neighbourhood.canSwap(*assignment, var1Index, value2Index);
        EXPECT_EQ(expected, actual);
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, Swap) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(vars), domainLb, domainUb, *solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*assignment, random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

    for (size_t var1Index = 0; var1Index < vars.size(); ++var1Index) {
      const Int value1 = solver->committedValue(vars.at(var1Index).solverId());
      for (size_t var2Index = 0; var2Index < vars.size(); ++var2Index) {
        if (var1Index == var2Index) {
          continue;
        }
        const Int value2 =
            solver->committedValue(vars.at(var2Index).solverId());
        EXPECT_GE(value2, domainLb);
        if (!setDomains.at(var1Index).contains(value2)) {
          continue;
        }
        const auto value2Index = static_cast<size_t>(value2 - domainLb);
        if (setDomains.at(var2Index).contains(value1)) {
          neighbourhood.swapValues(*assignment, annealer, var1Index,
                                   value2Index);
        }
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, AssignValue) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(vars), domainLb, domainUb, *solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*assignment, random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

    for (size_t varIndex = 0; varIndex < vars.size(); ++varIndex) {
      for (const Int newValue : domains.at(varIndex)) {
        const Int oldValue =
            solver->committedValue(vars.at(varIndex).solverId());
        EXPECT_GE(newValue, domainLb);
        bool freeValue = true;
        for (const auto& var : vars) {
          if (newValue == solver->committedValue(var.solverId())) {
            freeValue = false;
            break;
          }
        }
        if (!freeValue) {
          continue;
        }
        const auto newValueIndex = static_cast<size_t>(newValue - domainLb);
        EXPECT_EQ(oldValue,
                  solver->committedValue(vars.at(varIndex).solverId()));
        neighbourhood.assignValue(*assignment, annealer, varIndex,
                                  newValueIndex);
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, RandomMove) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(vars), domainLb, domainUb, *solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*assignment, random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(domains.size());
  for (size_t i = 0u; i < vars.size(); ++i) {
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
