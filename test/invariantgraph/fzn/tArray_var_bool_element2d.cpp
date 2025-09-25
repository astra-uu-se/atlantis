#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_var_bool_element2d.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_var_bool_element2dTest : public FznTestBase {
 public:
  std::vector<VarNodeId> inputVarNodeIds{};
  std::string rowIdx{"row_idx"};
  std::string colIdx{"col_idx"};
  Int numRows{0};
  Int rowOffset{1};
  Int colOffset{1};
  std::string output{"output"};
  std::vector<std::vector<std::string>> inputs{};

  void generate() override {
    numRows = *rc::gen::inRange(1, 3);
    const Int numCols = *rc::gen::inRange(1, 3);
    const bool useOffset = *rc::gen::arbitrary<bool>();

    constraintIdentifier = useOffset
                               ? "array_var_bool_element2d"
                               : "array_var_bool_element2d_nonshifted_flat";
    std::vector<std::string> flatMatrix;
    for (Int i = 0; i < numRows; i++) {
      inputs.emplace_back();
      for (Int j = 0; j < numCols; j++) {
        inputs.back().emplace_back("i_" + std::to_string(i) + '_' +
                                   std::to_string(j));
        flatMatrix.emplace_back(inputs.back().back());
      }
    }
    const Int rowIdxLb = *rc::gen::element(-1024, -1, 0, 1, 1024);
    addIntArg(rowIdxLb, numRows + rowIdxLb - 1, rowIdx);
    rowOffset = lowerBound(rowIdx);

    const Int colIdxLb = *rc::gen::element(-1024, -1, 0, 1, 1024);
    addIntArg(colIdxLb, numCols + colIdxLb - 1, colIdx);
    colOffset = lowerBound(colIdx);

    addBoolVarArray(flatMatrix);
    addBoolArg(output);
    addArg(numRows);
    addArg(rowOffset);
    addArg(colOffset);

    generateConstraint();
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const Int row = intVal(rowIdx, committedValue) - rowOffset;
    const Int col = intVal(colIdx, committedValue) - colOffset;
    const bool expected = boolVal(inputs.at(row).at(col));
    const bool actual = boolVal(output, committedValue);

    if (isFixed(output)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(output)) {
      return false;
    }
    const bool outVal = boolVal(output);
    if (isFixed(rowIdx) && isFixed(colIdx)) {
      const Int row = intVal(rowIdx) - rowOffset;
      const Int col = intVal(colIdx) - colOffset;
      const auto& input = inputs.at(row).at(col);
      return isFixed(input) && boolVal(input) != outVal;
    }
    if (isFixed(rowIdx)) {
      const Int row = intVal(rowIdx) - rowOffset;
      const auto& colDom = varNodeConst(colIdx).constDomain();
      return std::none_of(colDom->begin(), colDom->end(), [&](const Int val) {
        const Int col = val - colOffset;
        return !isFixed(inputs.at(row).at(col)) ||
               boolVal(inputs.at(row).at(col)) == outVal;
      });
    }
    if (isFixed(colIdx)) {
      const Int col = intVal(colIdx) - colOffset;
      const auto& rowDom = varNodeConst(rowIdx).constDomain();
      return std::none_of(rowDom->begin(), rowDom->end(), [&](const Int val) {
        const Int row = val - rowOffset;
        return !isFixed(inputs.at(row).at(col)) ||
               boolVal(inputs.at(row).at(col)) == outVal;
      });
    }
    const auto& rowDom = varNodeConst(rowIdx).constDomain();
    const auto& colDom = varNodeConst(colIdx).constDomain();
    return std::none_of(rowDom->begin(), rowDom->end(), [&](const Int rowIdx) {
      const Int row = rowIdx - rowOffset;
      return std::any_of(colDom->begin(), colDom->end(), [&](const Int colIdx) {
        const Int col = colIdx - colOffset;
        return !isFixed(inputs.at(row).at(col)) ||
               boolVal(inputs.at(row).at(col)) == outVal;
      });
    });
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (isFixed(rowIdx) && isFixed(colIdx)) {
      const Int row = intVal(rowIdx) - rowOffset;
      const Int col = intVal(colIdx) - colOffset;
      const auto& input = inputs.at(row).at(col);
      if (isFixed(input) && isFixed(output)) {
        return boolVal(input) == boolVal(output);
      }
      if (!isFixed(input) && !isFixed(output)) {
        return varNodeId(input) == varNodeId(output);
      }
      return false;
    }
    if (!isFixed(output)) {
      return false;
    }
    const bool outVal = boolVal(output);
    if (isFixed(rowIdx)) {
      const Int row = intVal(rowIdx) - rowOffset;
      const auto& colDom = varNodeConst(colIdx).constDomain();
      return std::all_of(colDom->begin(), colDom->end(), [&](const Int val) {
        const Int col = val - colOffset;
        return isFixed(inputs.at(row).at(col)) &&
               boolVal(inputs.at(row).at(col)) == outVal;
      });
    }
    if (isFixed(colIdx)) {
      const Int col = intVal(colIdx) - colOffset;
      const auto& rowDom = varNodeConst(rowIdx).constDomain();
      return std::all_of(rowDom->begin(), rowDom->end(), [&](const Int val) {
        const Int row = val - rowOffset;
        return isFixed(inputs.at(row).at(col)) &&
               boolVal(inputs.at(row).at(col)) == outVal;
      });
    }
    const auto& rowDom = varNodeConst(rowIdx).constDomain();
    const auto& colDom = varNodeConst(colIdx).constDomain();
    return std::all_of(rowDom->begin(), rowDom->end(), [&](const Int rowIdx) {
      const Int row = rowIdx - rowOffset;
      return std::all_of(colDom->begin(), colDom->end(), [&](const Int colIdx) {
        const Int col = colIdx - colOffset;
        return isFixed(inputs.at(row).at(col)) &&
               boolVal(inputs.at(row).at(col)) == outVal;
      });
    });
  }

  [[nodiscard]] bool canMove() const override {
    return varId(rowIdx) != propagation::NULL_ID ||
           std::ranges::any_of(inputs, [&](const auto& row) {
             return std::ranges::any_of(row, [&](const std::string& input) {
               return varId(input) != propagation::NULL_ID;
             });
           });
  }

  void move(bool committedValue) override {
    if (randBool()) {
      changeValue(rowIdx, committedValue);
    }
    for (const auto& row : inputs) {
      for (const auto& input : row) {
        if (varId(input) != propagation::NULL_ID && randBool()) {
          changeValue(input, committedValue);
        }
      }
    }
  }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(output));
  }
};

RC_GTEST_FIXTURE_PROP(array_var_bool_element2dTest, RapidCheck, ()) {
  rapidCheck();
}
}  // namespace atlantis::testing
