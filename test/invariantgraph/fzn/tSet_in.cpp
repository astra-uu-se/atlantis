#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/set_in.hpp"
#include "atlantis/sortedUniqueVector.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class set_inTest : public FznTestBase {
 public:
  std::string input{"input"};
  std::string domain{"domain"};
  std::string set{"set"};
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const auto& is = intSetVal(set);

    const bool expected = std::ranges::find(is, intVal(input, committedValue)) != is.end();
    const bool actual = boolVal(reified);

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  void generate() override {
    const auto is = *rc::gen::arbitrary<IntArgState>();
    auto dom = genDomain(is);
    addIntArg(is, dom, input);
    addIntSetPar(domain, std::move(dom));
    addIntSetArg(set);
    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "set_in_reif" : "set_in";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const auto& setVals = intSetVal(set);
    if (setVals.empty()) {
      return isFixedTo(reified, false);
    }
    if (!isFixed(reified)) {
      return false;
    }
    const auto& dom = intSetVal(domain);
    if (boolVal(reified)) {
      return std::ranges::any_of(dom, [&](const Int dVal) {
        return std::ranges::find(setVals, dVal) != setVals.end();
      });
    }
    return std::ranges::any_of(dom, [&](const Int dVal) {
      return std::ranges::find(setVals, dVal) == setVals.end();
    });
  }

  [[nodiscard]] bool neverSatisfied() const override {
    const auto& setVals = intSetVal(set);
    if (setVals.empty()) {
      return isFixedTo(reified, true);
    }
    if (!isFixed(reified)) {
      return false;
    }
    const auto& dom = intSetVal(domain);
    if (boolVal(reified)) {
      return std::ranges::all_of(dom, [&](const Int dVal) {
        return std::ranges::find(setVals, dVal) == setVals.end();
      });
    }
    return std::ranges::all_of(dom, [&](const Int dVal) {
      return std::ranges::find(setVals, dVal) != setVals.end();
    });
  }

  [[nodiscard]] bool canMove() const override {
    return varId(input) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    if (randBool()) {
      changeValue(input, committedValue);
    }
  }

  void query() override {
    if (varId(reified) != propagation::NULL_ID) {
      _solver->query(varId(reified));
    }
    _solver->query(totalViolationVarId());
  }
};

RC_GTEST_FIXTURE_PROP(set_inTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing