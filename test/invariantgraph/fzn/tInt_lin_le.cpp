#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <iostream>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_lin_le.hpp"

namespace atlantis::testing {

using rc::gen::inRange;
using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_lin_leTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::vector<Int> coeffs{};
  std::string reified{"reified"};
  Int bound{0};

  [[nodiscard]] std::pair<Int, Int> getBounds() const {
    Int lb = 0;
    Int ub = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      if (isFixed(inputs.at(i))) {
        lb += intVal(inputs.at(i)) * coeffs.at(i);
        ub += intVal(inputs.at(i)) * coeffs.at(i);
      } else {
        const Int vLb = lowerBound(inputs.at(i)) * coeffs.at(i);
        const Int vUb = upperBound(inputs.at(i)) * coeffs.at(i);
        lb += std::min<Int>(vLb, vUb);
        ub += std::max<Int>(vLb, vUb);
      }
    }
    return {lb, ub};
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) != 0) {
        sum += coeffs.at(i) * intVal(inputs.at(i), committedValue);
      }
    }
    const bool actual = boolVal(reified, committedValue);
    const bool expected = sum <= bound;

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const auto [lb, ub] = getBounds();
    const bool alwaysSat = ub <= bound;
    if (alwaysSat) {
      return isFixedTo(reified, bool{true});
    }
    const bool alwaysUnsat = bound < lb;
    if (alwaysUnsat) {
      return isFixedTo(reified, bool{false});
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    const bool alwaysSat = ub <= bound;
    if (alwaysSat) {
      return isFixedTo(reified, bool{false});
    }

    const bool alwaysUnsat = bound < lb;

    if (alwaysUnsat) {
      return isFixedTo(reified, bool{true});
    }

    return false;
  }

  void generate() override {
    const size_t size = *rc::gen::inRange<size_t>(0, 4);
    coeffs =
        *rc::gen::container<std::vector<Int>>(size, rc::gen::inRange(-2, 2));
    addArg(coeffs);
    inputs.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);

    Int lb = -1;
    Int ub = 2;
    for (const Int c : coeffs) {
      ub += std::max<Int>(0, c);
      lb += std::min<Int>(0, c);
    }

    bound = *rc::gen::inRange<Int>(lb, ub);
    addArg(bound);

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "int_lin_le_reif" : "int_lin_le";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputs, [&](const std::string& input) {
      return varId(input) != propagation::NULL_ID;
    });
  }

  void move(bool committedValue) override {
    for (const auto& input : inputs) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    if (varId(reified) != propagation::NULL_ID) {
      _solver->query(varId(reified));
    } else if (totalViolationVarId() != propagation::NULL_ID) {
      _solver->query(totalViolationVarId());
    }
  }
};

RC_GTEST_FIXTURE_PROP(int_lin_leTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
