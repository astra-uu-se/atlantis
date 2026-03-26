#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_table_int.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_table_intTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::string reified{"reified"};
  std::vector<std::vector<Int>> table{};

  void generate() override {
    const Int numVars = *rc::gen::inRange<Int>(1, 10);
    for (Int i = 0; i < numVars; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);

    table = *rc::gen::container<std::vector<std::vector<Int>>>(
        *rc::gen::inRange(0, 5), rc::gen::container<std::vector<Int>>(
                     numVars, rc::gen::inRange<Int>(-10, 10)));

    std::vector<Int> flatTable;
    flatTable.reserve(table.size() * numVars);
    for (const auto& row : table) {
      for (const Int col : row) {
        flatTable.emplace_back(col);
      }
    }

    addArg(flatTable);

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier =
        isReified ? "fzn_table_int_reif" : "fzn_table_int";

    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool isSatisfied(const bool committedValue) const override {
    std::vector<Int> vals(inputs.size());
    for (size_t i = 0; i < inputs.size(); i++) {
      vals.at(i) = intVal(inputs.at(i), committedValue);
    }
    bool expected = false;
    for (const auto& row : table) {
      bool satisfyingRow = true;
      for (size_t c = 0; c < row.size(); c++) {
        if (vals.at(c) != row.at(c)) {
          satisfyingRow = false;
          break;
        }
      }
      if (satisfyingRow) {
        expected = true;
        break;
      }
    }

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      const bool actual = boolVal(reified);
      return isSolution ? expected == actual : expected != actual;
    }
    const bool actual = boolVal(reified, committedValue);
    return expected == actual;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (inputs.empty()) {
      return true;
    }
    if (table.empty()) {
      return boolVal(reified);
    }
    std::vector<bool> validRows(table.size(), true);
    for (size_t r = 0; r < table.size(); r++) {
      for (size_t c = 0; c < inputs.size(); c++) {
        if (!inDomain(inputs.at(c), table.at(r).at(c))) {
          validRows.at(r) = false;
          break;
        }
      }
    }
    const size_t numValidRows = std::ranges::count_if(validRows, [&](const bool isValid) { return isValid; });
    if (numValidRows == 0) {
      return boolVal(reified);
    }
    const size_t numFree = std::ranges::count_if(inputs, [&](const std::string& input) { return !isFixed(input); });
    if (numFree <= 1 && boolVal(reified)) {
      return false;
    }
    if (boolVal(reified) && inputs.size() == 1) {
      return !boolVal(reified);
    }
    return false;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (table.empty()) {
      return !boolVal(reified);
    }
    if (boolVal(reified) && inputs.size() == 1) {
      return boolVal(reified);
    }
    std::vector<bool> validRows(table.size(), true);
    for (size_t r = 0; r < table.size(); r++) {
      for (size_t c = 0; c < inputs.size(); c++) {
        if (!inDomain(inputs.at(c), table.at(r).at(c))) {
          validRows.at(r) = false;
          break;
        }
      }
    }
    const size_t numValidRows = std::ranges::count_if(validRows, [&](const bool isValid) { return isValid; });
    if (numValidRows == 0) {
      return !boolVal(reified);
    }
    const size_t numFree = std::ranges::count_if(inputs, [&](const std::string& input) { return !isFixed(input); });
    if (numFree <= 1 && boolVal(reified)) {
      return true;
    }
    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputs, [&](const std::string& input) {
      return !isFixed(input);
    });
  }

  void move(const bool committedValue) override {
    for (const auto& input : inputs) {
      changeValue(input, committedValue);
    }
  }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(reified));
  }
};

RC_GTEST_FIXTURE_PROP(fzn_table_intTest, RapidCheck, ()) { rapidCheck(); }
}  // namespace atlantis::testing
