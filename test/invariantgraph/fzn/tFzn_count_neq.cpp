#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_neq.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_count_neqTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::string needle{"needle"};
  std::string output{"output"};
  std::string reified{"reified"};

  [[nodiscard]] std::pair<Int, Int> getBounds() const {
    Int lb = 0;
    Int ub = 0;
    for (const auto& input : inputs) {
      if (isFixed(input)) {
        const Int inputVal = intVal(input);
        if (isFixed(needle)) {
          if (inputVal == intVal(needle)) {
            ++lb;
            ++ub;
          }
        } else if (inDomain(needle, inputVal)) {
          ++ub;
        }
      } else if (isFixed(needle)) {
        if (inDomain(input, intVal(needle))) {
          ++ub;
        }
      } else if (!varNodeConst(input).constDomain()->isDisjoint(
                     *varNodeConst(needle).constDomain())) {
        ++ub;
      }
    }
    return {lb, ub};
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "FznCountEqTest::isSatisfied(" << to_string(committedValue)
             << ")" << std::endl;
    Int count = 0;
    if (!inputs.empty()) {
      const Int n = intVal(needle, committedValue);
      RC_LOG() << "needle = " << n << std::endl;
      for (const auto& input : inputs) {
        const Int i = intVal(input, committedValue);
        RC_LOG() << input << " = " << i << std::endl;
        if (i == n) {
          ++count;
        }
      }
    }
    RC_LOG() << "count = " << count << std::endl;
    const Int o = intVal(output, committedValue);
    RC_LOG() << "output = " << o << std::endl;
    const bool expected = count != o;
    RC_LOG() << "expected = " << to_string(expected) << std::endl;
    const bool actual = boolVal(reified, committedValue);
    RC_LOG() << "actual = " << to_string(actual) << std::endl;

    if (isFixed(reified)) {
      const bool satAssignment = violation(committedValue) == 0;
      RC_LOG() << "satAssignment = " << to_string(satAssignment) << std::endl;
      return satAssignment ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    if (lb == ub) {
      if (isFixedTo(reified, true)) {
        return !isFixedTo(output, lb);
      }
      return inDomain(output, lb);
    }
    const bool alwaysSat = ub < lowerBound(output) || upperBound(output) < lb;
    if (alwaysSat) {
      return isFixedTo(reified, bool{true});
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    if (lb == ub) {
      if (isFixedTo(reified, true)) {
        return isFixedTo(output, lb);
      }
      return !inDomain(output, lb);
    }
    const bool alwaysSat = ub < lowerBound(output) || upperBound(output) < lb;
    if (alwaysSat) {
      return isFixedTo(reified, bool{false});
    }
    return false;
  }

  void generate() override {
    const size_t size = *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);
    addIntArg(needle);
    addIntArg(output);

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "fzn_count_neq_reif" : "fzn_count_neq";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return varId(needle) != propagation::NULL_ID ||
           std::ranges::any_of(inputs, [&](const std::string& input) {
             return varId(input) != propagation::NULL_ID;
           });
  }

  void move(bool committedValue) override {
    if (varId(needle) != propagation::NULL_ID && randBool()) {
      changeValue(needle, committedValue);
    }
    for (const auto& input : inputs) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    for (const auto& vId :
         std::array{varId(reified), varId(output), totalViolationVarId()}) {
      if (vId != propagation::NULL_ID) {
        _solver->query(vId);
      }
    }
  }
};

RC_GTEST_FIXTURE_PROP(fzn_count_neqTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
