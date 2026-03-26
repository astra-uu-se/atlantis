#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_eq.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_count_eqTest : public FznTestBase {
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
      const Int n = intVal(needle);
      RC_LOG() << "needle = " << n << std::endl;
      for (const auto& input : inputs) {
        const Int i = intVal(input, committedValue);
        RC_LOG() << input << " = " << i << std::endl;
        if (i == n) {
          ++count;
        }
      }
    }

    const Int o = intVal(output, committedValue);
    const bool expected = count == o;
    const bool actual = boolVal(reified, committedValue);

    RC_LOG() << "output = " << o << std::endl;
    RC_LOG() << "count = " << count << std::endl;
    RC_LOG() << "expected = " << to_string(expected) << std::endl;
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
        return isFixedTo(output, lb);
      }
      return !inDomain(output, lb);
    }
    const bool alwaysUnsat = ub < lowerBound(output) || upperBound(output) < lb;
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
    if (lb == ub) {
      if (isFixedTo(reified, false)) {
        return isFixedTo(output, lb);
      }
      return !inDomain(output, lb);
    }
    const bool alwaysUnsat = ub < lowerBound(output) || upperBound(output) < lb;
    if (alwaysUnsat) {
      return isFixedTo(reified, bool{true});
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
    constraintIdentifier = isReified ? "fzn_count_eq_reif" : "fzn_count_eq";
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
    std::unordered_set<InvariantNodeId, InvariantNodeIdHash>
        implicitConstraints;
    std::vector<bool> hasImplicitConstraints(inputs.size(), false);
    implicitConstraints.reserve(inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
      if (!isFixed(inputs.at(i))) {
        const auto& defNodes = varNodeConst(inputs.at(i)).definingNodes();
        if (!defNodes.empty()) {
          RC_ASSERT(defNodes.size() == size_t{1});
          const InvariantNodeId implId = *defNodes.begin();
          RC_ASSERT(implId.isImplicitConstraint());
          implicitConstraints.emplace(implId);
          hasImplicitConstraints.at(i) = true;
        }
      }
    }

    for (size_t i = 0; i < inputs.size(); ++i) {
      if (!hasImplicitConstraints.at(i) && !isFixed(inputs.at(i)) &&
          randBool()) {
        changeValue(inputs.at(i), committedValue);
      }
    }

    for (const InvariantNodeId implId : implicitConstraints) {
      auto implNode = _solverMapping->neighborhood(implId);
      RC_ASSERT(implNode != nullptr);
      implNode->randomMove(*_randomProvider, *_assignment);
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

RC_GTEST_FIXTURE_PROP(fzn_count_eqTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
