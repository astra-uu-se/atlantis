#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "atlantis/invariantgraph/fzn/array_var_bool_element2d.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_var_bool_element2dTest : public ::testing::Test {
 public:
  std::vector<VarNodeId> inputVarNodeIds{};
  Int numInputs = 3;

  void SetUp() override {}

  /*
    [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(output)) {
      return false;
      const auto& outputNode = _invariantGraph->varNode(output);
      if (!isFixed(rowIndex)) {
        const auto& rowIdxNode = _invariantGraph->varNode(rowIndex);
        return std::none_of(
            rowIdxNode.constDomain()->begin(), rowIdxNode.constDomain()->end(),
            [&](const Int rowVal) {
              if (!isFixed(colIndex)) {
                const auto& colIdxNode = _invariantGraph->varNode(colIndex);
                return std::none_of(colIdxNode.constDomain()->begin(),
                                    colIdxNode.constDomain()->end(),
                                    [&](const Int colVal) {
                                      return outputNode.inDomain(
                                          getValue(rowVal, colVal));
                                    });
              }
              return outputNode.inDomain(
                  getValue(rowVal, intVal(colIndex)));
            });
      }
      if (!isFixed(colIndex)) {
        const auto& colIdxNode = _invariantGraph->varNode(colIndex);
        return std::none_of(colIdxNode.constDomain()->begin(),
                            colIdxNode.constDomain()->end(),
                            [&](const Int colVal) {
                              return outputNode.inDomain(
                                  getValue(intVal(rowIndex), colVal));
                            });
      }
      return outputNode.inDomain(
          getValue(intVal(rowIndex), intVal(colIndex)));
    }
    if (!isFixed(rowIndex)) {
      const auto& rowIdxNode = _invariantGraph->varNode(rowIndex);
      if (!isFixed(colIndex)) {
        return std::none_of(
            rowIdxNode.constDomain()->begin(), rowIdxNode.constDomain()->end(),
            [&](const Int rowVal) {
              if (!isFixed(colIndex)) {
                const auto& colIdxNode = _invariantGraph->varNode(colIndex);
                return std::none_of(
                    colIdxNode.constDomain()->begin(),
                    colIdxNode.constDomain()->end(), [&](const Int colVal) {
                      return boolVal(output) == getValue(rowVal, colVal);
                    });
              }
              return boolVal(output) == getValue(rowVal, intVal(colIndex));
            });
      }
      return std::none_of(
          rowIdxNode.constDomain()->begin(), rowIdxNode.constDomain()->end(),
          [&](const Int rowVal) {
            return getValue(rowVal, intVal(colIndex)) == boolVal(output);
          });
    }
    if (!isFixed(colIndex)) {
      const auto& colIdxNode = _invariantGraph->varNode(colIndex);
      return std::none_of(
          colIdxNode.constDomain()->begin(), colIdxNode.constDomain()->end(),
          [&](const Int colVal) {
            return getValue(intVal(colIndex), colVal) == boolVal(output);
          });
    }
    const bool expected = getValue(intVal(rowIndex), intVal(colIndex));
    const bool actual = boolVal(output);
    return expected != actual;
  }*/
};

}  // namespace atlantis::testing
