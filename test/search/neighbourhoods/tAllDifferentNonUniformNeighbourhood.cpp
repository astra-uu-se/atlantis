#include <gtest/gtest.h>

#include <unordered_set>

#include "./testHelper.hpp"
#include "atlantis/search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighbourhoods;

class AllDifferentNonUniformNeighbourhoodTest : public NeighbourhoodTestBase {
 public:
  std::shared_ptr<AllDifferentNonUniformNeighbourhood> _neighbourhood;

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

  void expectHolds() {
    if (_vars.empty()) {
      return;
    }
    std::unordered_set<Int> curVals;
    curVals.reserve(_vars.size());
    std::unordered_set<Int> comVals;
    comVals.reserve(_vars.size());

    for (const auto& var : _vars) {
      const Int curVal = _solver->currentValue(var.solverId());
      const Int comVal = _solver->committedValue(var.solverId());
      EXPECT_TRUE(var.constDomain().contains(curVal));
      EXPECT_TRUE(var.constDomain().contains(comVal));

      EXPECT_FALSE(curVals.contains(curVal));
      EXPECT_FALSE(comVals.contains(comVal));

      curVals.emplace(curVal);
      comVals.emplace(comVal);
    }
  }

  void SetUp() override {
    NeighbourhoodTestBase::SetUp();
    _solver->open();
    for (const auto& domain : _domains) {
      const auto& [lb, ub] = std::minmax_element(domain.begin(), domain.end());

      propagation::VarViewId var = _solver->makeIntVar(*lb, *lb, *ub);
      domainLb = std::min(domainLb, *lb);
      domainUb = std::max(domainUb, *ub);
      _vars.emplace_back(var, SearchDomain(domain));
    }
    _solver->close();

    _neighbourhood = std::make_shared<AllDifferentNonUniformNeighbourhood>(
        std::vector<SearchVar>(_vars), domainLb, domainUb);
  }
};

TEST_F(AllDifferentNonUniformNeighbourhoodTest, initialize) {
  initialize(*_neighbourhood);
  expectHolds();

  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _neighbourhood->initialize(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, canSwap) {
  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  initialize(*_neighbourhood);
  expectHolds();
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

TEST_F(AllDifferentNonUniformNeighbourhoodTest, swap) {
  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    for (size_t var1Index = 0; var1Index < _vars.size(); ++var1Index) {
      initialize(*_neighbourhood);
      expectHolds();
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

TEST_F(AllDifferentNonUniformNeighbourhoodTest, assignValue) {
  std::vector<std::unordered_set<Int>> setDomains(_domains.size());
  for (size_t i = 0u; i < _vars.size(); ++i) {
    setDomains.at(i) = std::unordered_set<Int>();
    for (const Int val : _domains.at(i)) {
      setDomains.at(i).emplace(val);
    }
  }
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    initialize(*_neighbourhood);

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

TEST_F(AllDifferentNonUniformNeighbourhoodTest, randomMove) {
  initialize(*_neighbourhood);
  expectHolds();

  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    const size_t actual = _neighbourhood->randomMove(_random, *_assignment);
    EXPECT_GE(actual, 1);
    EXPECT_LE(actual, 2);
    expectHolds();
  }
}

TEST_F(AllDifferentNonUniformNeighbourhoodTest, commitIf) {
  initialize(*_neighbourhood);
  expectHolds();
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    commitIf(*_neighbourhood);
    expectHolds();
  }
}

}  // namespace atlantis::testing
