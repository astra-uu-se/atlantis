#include <gtest/gtest.h>

#include <unordered_set>

#include "../testHelper.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighbourhoods;

class AllDifferentNonUniformNeighbourhoodTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<AllDifferentNonUniformNeighbourhood> _neighbourhood;
  std::shared_ptr<Assignment> _assignment;
  RandomProvider _random{123456789};

  std::vector<SearchVar> _vars;
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
    for (const auto& domain : _domains) {
      const auto& [lb, ub] = std::minmax_element(domain.begin(), domain.end());

      propagation::VarViewId var = _solver->makeIntVar(*lb, *lb, *ub);
      domainLb = std::min(domainLb, *lb);
      domainUb = std::max(domainUb, *ub);
      _vars.emplace_back(var, SearchDomain(domain));
    }

    _neighbourhood = std::make_shared<AllDifferentNonUniformNeighbourhood>(
        std::vector<SearchVar>(_vars), domainLb, domainUb);

    _assignment = std::make_unique<Assignment>(
        *_solver, *_neighbourhood, _solver->makeIntVar(0, 0, 0),
        _solver->makeIntVar(0, 0, 0), ObjectiveDirection::NONE, 0);

    _solver->close();
  }
};

TEST_F(AllDifferentNonUniformNeighbourhoodTest, Initialize) {
  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }

  std::unordered_set<Int> usedValues{};
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _assignment->initialise(_random);
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
  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  _assignment->initialise(_random);
  for (size_t var1Index = 0; var1Index < _vars.size(); ++var1Index) {
    const Int value1 = _solver->committedValue(_vars.at(var1Index).solverId());
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
          _neighbourhood->canSwap(*_assignment, var1Index, value2Index);
      EXPECT_EQ(expected, actual);
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, Swap) {
  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  _assignment->initialise(_random);
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
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
          _neighbourhood->swapValues(*_assignment, var1Index, value2Index);
        }
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, AssignValue) {
  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _assignment->initialise(_random);

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
        _neighbourhood->assignValue(*_assignment, varIndex, newValueIndex);
      }
    }
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, RandomMove) {
  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  _assignment->initialise(_random);
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    EXPECT_GT(_neighbourhood->randomMove(_random, *_assignment), size_t{0});
  }
}
}  // namespace atlantis::testing
