#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_element2d.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_bool_element2dTest : public FznTestBase {
 public:
  std::vector<VarNodeId> inputVarNodeIds{};
  std::string rowIndex{"rowIndex"};
  std::string colIndex{"colIndex"};
  std::vector<std::vector<bool>> parameters{};
  std::string output{"output"};
  Int rowOffset{1};
  Int colOffset{1};

  bool getValue(Int rowValue, Int colValue) const {
    return parameters.at(rowValue - rowOffset).at(colValue - colOffset);
  }

  void generate() override {
    const Int numRows = true ? 2 : *rc::gen::inRange(1, 3);
    const Int numCols = true ? 2 : *rc::gen::inRange(1, 3);

    constraintIdentifier = false && *rc::gen::arbitrary<bool>()
                               ? "array_bool_element2d"
                               : "array_bool_element2d_nonshifted_flat";

    const Int rowLb = true ? -1024 : *rc::gen::element(-1024, -1, 0, 1, 1024);
    addIntArg(IntArgState::VAR, rowLb, numRows + rowLb - 1, rowIndex);

    const Int colLb = true ? 0 : *rc::gen::element(-1024, -1, 0, 1, 1024);
    addIntArg(IntArgState::VAR, colLb, numCols + colLb - 1, colIndex);

    parameters =
        true ? std::vector<std::vector<bool>>{{true, false}, {false, false}}
             : *rc::gen::container<std::vector<std::vector<bool>>>(
                   numRows, rc::gen::container<std::vector<bool>>(
                                numCols, rc::gen::arbitrary<bool>()));

    std::vector<bool> flatPars;
    flatPars.reserve(numRows * numCols);
    for (Int row = 0; row < numRows; row++) {
      for (Int col = 0; col < numCols; col++) {
        flatPars.emplace_back(parameters.at(row).at(col));
      }
    }

    addArg(flatPars);

    addBoolArg(BoolArgState::VAR, output);

    addArg(numRows);
    rowOffset = lowerBound(rowIndex);
    addArg(rowOffset);
    colOffset = lowerBound(colIndex);
    addArg(colOffset);

    generateConstraint();
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const Int rowIdxVal = intVal(rowIndex, committedValue);
    const Int colIdxVal = intVal(colIndex, committedValue);
    const bool expected =
        parameters.at(rowIdxVal - rowOffset).at(colIdxVal - colOffset);
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
    if (!isFixed(rowIndex)) {
      const auto& rowIdxNode = varNodeConst(rowIndex);
      if (!isFixed(colIndex)) {
        const auto& colIdxNode = varNodeConst(colIndex);
        return std::none_of(
            rowIdxNode.constDomain()->begin(), rowIdxNode.constDomain()->end(),
            [&](const Int rowVal) {
              return std::any_of(
                  colIdxNode.constDomain()->begin(),
                  colIdxNode.constDomain()->end(), [&](const Int colVal) {
                    return boolVal(output) == getValue(rowVal, colVal);
                  });
            });
      }
      return std::none_of(
          rowIdxNode.constDomain()->begin(), rowIdxNode.constDomain()->end(),
          [&](const Int rowVal) {
            return getValue(rowVal, intVal(colIndex)) == boolVal(output);
          });
    }
    if (!isFixed(colIndex)) {
      const auto& colIdxNode = varNodeConst(colIndex);
      return std::none_of(
          colIdxNode.constDomain()->begin(), colIdxNode.constDomain()->end(),
          [&](const Int colVal) {
            return getValue(intVal(rowIndex), colVal) == boolVal(output);
          });
    }
    const bool expected = getValue(intVal(rowIndex), intVal(colIndex));
    const bool actual = boolVal(output);
    return expected != actual;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(output)) {
      return false;
    }
    if (!isFixed(rowIndex)) {
      const auto& rowIdxNode = varNodeConst(rowIndex);
      if (!isFixed(colIndex)) {
        const auto& colIdxNode = varNodeConst(colIndex);
        return std::all_of(
            rowIdxNode.constDomain()->begin(), rowIdxNode.constDomain()->end(),
            [&](const Int rowVal) {
              return std::all_of(
                  colIdxNode.constDomain()->begin(),
                  colIdxNode.constDomain()->end(), [&](const Int colVal) {
                    return boolVal(output) == getValue(rowVal, colVal);
                  });
            });
      }
      return std::all_of(
          rowIdxNode.constDomain()->begin(), rowIdxNode.constDomain()->end(),
          [&](const Int rowVal) {
            return getValue(rowVal, intVal(colIndex)) == boolVal(output);
          });
    }
    if (!isFixed(colIndex)) {
      const auto& colIdxNode = varNodeConst(colIndex);
      return std::all_of(
          colIdxNode.constDomain()->begin(), colIdxNode.constDomain()->end(),
          [&](const Int colVal) {
            return getValue(intVal(rowIndex), colVal) == boolVal(output);
          });
    }
    return getValue(intVal(rowIndex), intVal(colIndex)) == boolVal(output);
  }

  [[nodiscard]] bool canMove() const override {
    return !isFixed(rowIndex) || !isFixed(colIndex);
  }

  void move(bool committedValue) override {
    if (!isFixed(rowIndex) && randBool()) {
      changeValue(rowIndex, committedValue);
    }
    if (!isFixed(colIndex) && randBool()) {
      changeValue(colIndex, committedValue);
    }
  }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(output));
  }
};

RC_GTEST_FIXTURE_PROP(array_bool_element2dTest, RapidCheck, ()) {
  rapidCheck();
}
}  // namespace atlantis::testing
