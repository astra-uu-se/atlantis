#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/tableNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

class TableNeighborhoodTest
    : public NeighborhoodTestBase<TableNeighborhood> {
 public:
  std::vector<SearchVar> _vars;
  std::vector<std::vector<Int>> _table;


  void expectHolds() {
    std::vector<Int> curVals(_vars.size());
    std::vector<Int> comVals(_vars.size());

    for (size_t i = 0; i < _vars.size(); ++i) {
      curVals.at(i) = _solver->currentValue(_vars.at(i).solverId());
      comVals.at(i) = _solver->committedValue(_vars.at(i).solverId());
    }
    Int curRow = -1;
    Int comRow = -1;
    for (size_t r = 0; r < _table.size(); ++r) {
      bool curSameRow = true;
      bool comSameRow = true;
      for (size_t c = 0; c < _table.at(r).size(); ++c) {
        if (_table.at(r).at(c) != curVals.at(c)) {
          curSameRow = false;
        }
        if (_table.at(r).at(c) != comVals.at(c)) {
          comSameRow = false;
        }
      }
      if (curSameRow) {
        curRow = static_cast<Int>(r);
      }
      if (comSameRow) {
        comRow = static_cast<Int>(r);
      }
    }
    EXPECT_GE(curRow, 0);
    EXPECT_GE(comRow, 0);
  }

  void SetUp() override {
    NeighborhoodTestBase::SetUp();
    _solver->open();

    for (size_t i = 0; i < 4; ++i) {
      propagation::VarViewId var = _solver->makeIntVar(1, 1, 5);
      _vars.emplace_back(var, std::make_shared<SearchDomain>(1, 5));
    }
    _table.resize(5, std::vector<Int>(_vars.size()));
    for (size_t r = 0; r < _table.size(); ++r) {
      for (size_t c = 0; c < _vars.size(); ++c) {
        _table.at(r).at(c) = (static_cast<Int>(r + c) % 5) + 1;
      }
    }
    _solver->close();

    createNeighborhood(std::vector<SearchVar>(_vars), std::vector<std::vector<Int>>(_table));
  }
};

TEST_F(TableNeighborhoodTest, initialize) {
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    initialize();
    expectHolds();
  }
}

TEST_F(TableNeighborhoodTest, randomMove) {
  initialize();
  expectHolds();
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    commitIf();
    expectHolds();
  }
}

}  // namespace atlantis::testing
