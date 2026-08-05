#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_int_maximum.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace fznparser;
using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_int_maximumTest : public FznTestBase {
 public:
  std::vector<std::string> inputs;
  std::string output{"output"};

  Int getLb() const {
    Int lb = lowerBound(inputs.front());
    for (size_t i = 1; i < inputs.size(); ++i) {
      lb = std::max(lb, lowerBound(inputs.at(i)));
    }
    return lb;
  }

  Int getUb() const {
    Int ub = upperBound(inputs.front());
    for (size_t i = 1; i < inputs.size(); ++i) {
      ub = std::max(ub, upperBound(inputs.at(i)));
    }
    return ub;
  }

  Int getValue(const bool committedValue) const {
    Int result = intVal(inputs.front(), committedValue);
    for (size_t i = 1; i < inputs.size(); ++i) {
      const Int v = intVal(inputs.at(i), committedValue);
      result = std::max(result, v);
    }
    return result;
  }

  void generate() override {
    constraintIdentifier = "array_int_maximum";
    addIntArg(output);
    const size_t size = *rc::gen::inRange(1, 3);
    for (size_t i = 0; i < size; i++) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);
    generateConstraint();
    markOutputVar(output);
  }

  [[nodiscard]] bool isSatisfied(const bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "array_int_maximum::isSatisfied(" << to_string(committedValue)
             << ")" << std::endl;

    const Int expected = getValue(committedValue);
    const Int actual = intVal(output, committedValue);

    RC_LOG() << "actual = " << actual << std::endl;
    RC_LOG() << "expected = " << expected << std::endl;

    if (isFixed(output)) {
      const bool satAssignment = violation(committedValue) == 0;
      RC_LOG() << "satAssignment = " << to_string(satAssignment) << std::endl;
      return satAssignment ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    return upperBound(output) < getLb() || getUb() < lowerBound(output);
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const size_t numCandidates =
        std::ranges::count_if(inputs, [&](const std::string& input) {
          return !isFixed(input) && lowerBound(input) <= lowerBound(output) &&
                 upperBound(output) <= upperBound(input);
        });
    return numCandidates <= 1;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(
        inputs, [&](const std::string& input) { return !isFixed(input); });
  }

  void move(const bool committedValue) override {
    for (const auto& input : inputs) {
      if (!isFixed(input) && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(output));
  }
};

RC_GTEST_FIXTURE_PROP(array_int_maximumTest, RapidCheck, ()) { rapidCheck(); }
}  // namespace atlantis::testing
