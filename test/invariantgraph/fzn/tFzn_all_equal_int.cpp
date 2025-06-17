#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_equal_int.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_all_equal_intTest : public FznTestBase {
 public:
  std::vector<std::string> inputs;
  std::string reified{"reified"};

  [[nodiscard]] bool getValue(bool committedValue) const {
    std::vector<Int> vals;
    vals.reserve(inputs.size());
    for (const auto& input : inputs) {
      const Int v = intVal(input, committedValue);
      vals.emplace_back(v);
      RC_LOG() << input << " = " << v << std::endl;
    }
    for (size_t i = 0; i < inputs.size() - 1; ++i) {
      if (vals.at(i) != vals.at(i + 1)) {
        RC_LOG() << inputs.at(i) << " != " << inputs.at(i + 1) << " ("
                 << vals.at(i) << " != " << vals.at(i + 1) << ')' << std::endl;
        return false;
      }
    }
    RC_LOG() << "all equal (" << vals.front() << ')' << std::endl;
    return true;
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "Fzn_all_equal_intTest::isSatisfied("
             << to_string(committedValue) << ')' << std::endl;
    const bool expected = getValue(committedValue);
    const bool actual = boolVal(reified, committedValue);

    RC_LOG() << "expected = " << to_string(expected) << std::endl;
    RC_LOG() << "actual = " << to_string(actual) << std::endl;

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      RC_LOG() << "isSolution = " << to_string(isSolution) << std::endl;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  void generate() override {
    const size_t size = *rc::gen::inRange(1, 10);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);
    // two vars corresponds to all_different
    const bool isReified = size == 2 ? false : *rc::gen::arbitrary<bool>();
    constraintIdentifier =
        isReified ? "fzn_all_equal_int_reif" : "fzn_all_equal_int";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (boolVal(reified)) {
      if (inputs.size() <= 1) {
        return true;
      }
      Int unionLb = std::numeric_limits<Int>::max();
      Int unionUb = std::numeric_limits<Int>::min();
      for (const auto& input : inputs) {
        unionLb = std::min(unionLb, lowerBound(input));
        unionUb = std::max(unionUb, upperBound(input));
      }
      if (unionLb == unionUb) {
        return true;
      }
    }
    Int overlapLb = std::numeric_limits<Int>::min();
    Int overlapUb = std::numeric_limits<Int>::max();
    for (const auto& input : inputs) {
      overlapLb = std::max(overlapLb, lowerBound(input));
      overlapUb = std::min(overlapUb, upperBound(input));
    }
    if (overlapLb > overlapUb) {
      return true;
    }
    SearchDomain overlap(overlapLb, overlapUb);
    try {
      for (const auto& input : inputs) {
        if (isFixed(input)) {
          overlap.fix(intVal(input));
        } else {
          overlap.intersect(*varNodeConst(input).constDomain());
        }
      }
    } catch (const InconsistencyException&) {
      return true;
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (boolVal(reified)) {
      if (inputs.size() <= 1) {
        return false;
      }
      Int overlapLb = std::numeric_limits<Int>::min();
      Int overlapUb = std::numeric_limits<Int>::max();
      for (const auto& input : inputs) {
        overlapLb = std::max(overlapLb, lowerBound(input));
        overlapUb = std::min(overlapUb, upperBound(input));
      }
      if (overlapLb > overlapUb) {
        return true;
      }
      SearchDomain overlap(overlapLb, overlapUb);
      try {
        for (const auto& input : inputs) {
          if (isFixed(input)) {
            overlap.fix(intVal(input));
          } else {
            overlap.intersect(*varNodeConst(input).constDomain());
          }
        }
      } catch (const InconsistencyException&) {
        return true;
      }
      return false;
    }
    if (inputs.size() <= 1) {
      return true;
    }
    Int unionLb = std::numeric_limits<Int>::max();
    Int unionUb = std::numeric_limits<Int>::min();
    for (const auto& input : inputs) {
      unionLb = std::min(unionLb, lowerBound(input));
      unionUb = std::max(unionUb, upperBound(input));
    }
    if (unionLb == unionUb) {
      return true;
    }
    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(
        inputs, [&](const std::string& input) { return !isFixed(input); });
  }

  void move(bool committedValue) override {
    for (const auto& input : inputs) {
      if (!isFixed(input) && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(reified));
  }
};

RC_GTEST_FIXTURE_PROP(fzn_all_equal_intTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing