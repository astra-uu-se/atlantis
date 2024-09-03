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
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<search::Assignment> _assignment;
  search::RandomProvider _random{123456789};

  std::vector<search::SearchVar> _vars;
  std::vector<std::vector<Int>> _domains{
      std::vector<Int>{1, 3, 4},
      std::vector<Int>{1, 4},
      std::vector<Int>{2, 4, 5},
  };

  Int domainLb{
      *std::min_element(_domains.front().begin(), _domains.front().end())};
  Int domainUb{
      *std::max_element(_domains.front().begin(), _domains.front().end())};

  void SetUp() override {
    _solver = std::make_unique<propagation::Solver>();
    _solver->open();
    _assignment = std::make_unique<search::Assignment>(
        *_solver, _solver->makeIntVar(0, 0, 0), _solver->makeIntVar(0, 0, 0),
        propagation::ObjectiveDirection::NONE, 0);
    for (const auto& domain : _domains) {
      const auto& [lb, ub] = std::minmax_element(domain.begin(), domain.end());

      propagation::VarId var = _solver->makeIntVar(*lb, *lb, *ub);
      domainLb = std::min(domainLb, *lb);
      domainUb = std::max(domainUb, *ub);
      _vars.emplace_back(var, SearchDomain(domain));
    }
    _solver->close();
  }
};

TEST_F(AllDifferentNonUniformNeighbourhoodTest, Initialize) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(_vars), domainLb, domainUb, *_solver);

  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }

  std::unordered_set<Int> usedValues{};
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });
    usedValues.clear();
    for (auto i = 0u; i < _vars.size(); ++i) {
      const Int value = _solver->committedValue(_vars.at(i).solverId());
      EXPECT_FALSE(usedValues.contains(value));
      usedValues.emplace(value);
      EXPECT_TRUE(setDomains.at(i).contains(value));
    }
    EXPECT_EQ(usedValues.size(), _vars.size());
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, CanSwap) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(_vars), domainLb, domainUb, *_solver);

  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

    for (size_t var1Index = 0; var1Index < _vars.size(); ++var1Index) {
      const Int value1 =
          _solver->committedValue(_vars.at(var1Index).solverId());
      for (size_t var2Index = 0; var2Index < _vars.size(); ++var2Index) {
        if (var1Index == var2Index) {
          continue;
        }
        const Int value2 =
            _solver->committedValue(_vars.at(var2Index).solverId());
        EXPECT_GE(value2, domainLb);
        if (!setDomains.at(var1Index).contains(value2)) {
          continue;
        }
        const auto value2Index = static_cast<size_t>(value2 - domainLb);
        bool expected = setDomains.at(var2Index).contains(value1);
        bool actual =
            neighbourhood.canSwap(*_assignment, var1Index, value2Index);
        EXPECT_EQ(expected, actual);
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, Swap) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(_vars), domainLb, domainUb, *_solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*_assignment, _random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

    for (size_t var1Index = 0; var1Index < _vars.size(); ++var1Index) {
      const Int value1 =
          _solver->committedValue(_vars.at(var1Index).solverId());
      for (size_t var2Index = 0; var2Index < _vars.size(); ++var2Index) {
        if (var1Index == var2Index) {
          continue;
        }
        const Int value2 =
            _solver->committedValue(_vars.at(var2Index).solverId());
        EXPECT_GE(value2, domainLb);
        if (!setDomains.at(var1Index).contains(value2)) {
          continue;
        }
        const auto value2Index = static_cast<size_t>(value2 - domainLb);
        if (setDomains.at(var2Index).contains(value1)) {
          neighbourhood.swapValues(*_assignment, annealer, var1Index,
                                   value2Index);
        }
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, AssignValue) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(_vars), domainLb, domainUb, *_solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*_assignment, _random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

    for (size_t varIndex = 0; varIndex < _vars.size(); ++varIndex) {
      for (const Int newValue : _domains.at(varIndex)) {
        const Int oldValue =
            _solver->committedValue(_vars.at(varIndex).solverId());
        EXPECT_GE(newValue, domainLb);
        bool freeValue = true;
        for (const auto& var : _vars) {
          if (newValue == _solver->committedValue(var.solverId())) {
            freeValue = false;
            break;
          }
        }
        if (!freeValue) {
          continue;
        }
        const auto newValueIndex = static_cast<size_t>(newValue - domainLb);
        EXPECT_EQ(oldValue,
                  _solver->committedValue(_vars.at(varIndex).solverId()));
        neighbourhood.assignValue(*_assignment, annealer, varIndex,
                                  newValueIndex);
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, RandomMove) {
  search::neighbourhoods::AllDifferentNonUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(_vars), domainLb, domainUb, *_solver);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*_assignment, _random, *schedule);

  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

    EXPECT_TRUE(neighbourhood.randomMove(_random, *_assignment, annealer));
  }
}
}  // namespace atlantis::testing
