#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

// rapid check: seed=2973283369362366998
class fzn_all_different_intTest : public FznTestBase {
 public:
  std::vector<std::string> inputs;
  std::string reified{"reified"};

  bool getValue(bool committedValue) const {
    for (size_t i = 0; i < inputs.size(); i++) {
      const Int val = intVal(inputs.at(i), committedValue);
      for (size_t j = i + 1; j < inputs.size(); j++) {
        if (val == intVal(inputs.at(j), committedValue)) {
          return false;
        }
      }
    }
    return true;
  }

  void generate() override {
    const Int numVars = *rc::gen::inRange<Int>(0, 10);
    for (Int i = 0; i < numVars; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier =
        isReified ? "fzn_all_different_int_reif" : "fzn_all_different_int";

    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected = getValue(committedValue);
    const bool actual = boolVal(reified, committedValue);

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  std::vector<std::unordered_set<Int>> getDomains() const {
    std::vector<std::unordered_set<Int>> domains;
    domains.reserve(inputs.size());
    for (const auto& var : inputs) {
      domains.emplace_back();
      if (isFixed(var)) {
        domains.back().emplace(intVal(var));
        continue;
      }
      const auto& vNode = varNodeConst(var);
      domains.back().reserve(vNode.constDomain()->size());
      for (const Int val : *vNode.constDomain()) {
        domains.back().emplace(val);
      }
    }
    std::vector<size_t> singletons;
    singletons.reserve(inputs.size());
    for (size_t i = 0; i < inputs.size(); i++) {
      if (domains.at(i).size() == 1) {
        singletons.emplace_back(i);
      }
    }
    for (size_t index = 0; index < singletons.size(); ++index) {
      const size_t i = singletons.at(index);
      RC_ASSERT(domains.at(i).size() <= size_t{1});
      if (domains.at(i).empty()) {
        continue;
      }
      const Int val = *domains.at(i).begin();

      for (size_t j = 0; j < inputs.size(); j++) {
        if (j != i) {
          const size_t prevSize = domains.at(j).size();
          domains.at(j).erase(val);
          if (prevSize != 1 && domains.at(j).size() == 1) {
            singletons.emplace_back(j);
          }
        }
      }
    }
    return domains;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (boolVal(reified)) {
      if (inputs.size() <= 1) {
        return false;
      }
      Int unionLb = std::numeric_limits<Int>::max();
      Int unionUb = std::numeric_limits<Int>::min();
      for (const auto& input : inputs) {
        unionLb = std::min(unionLb, lowerBound(input));
        unionUb = std::max(unionUb, upperBound(input));
      }
      if (unionUb - unionLb + 1 < static_cast<Int>(inputs.size())) {
        return true;
      }
      std::vector<std::unordered_set<Int>> domains = getDomains();
      return std::ranges::any_of(domains,
                                 [&](const auto& dom) { return dom.empty(); });
    }
    if (inputs.size() <= 1) {
      return true;
    }
    std::unordered_set<Int> unionDom;
    unionDom.reserve(inputs.size());
    for (const auto& input : inputs) {
      if (isFixed(input)) {
        const Int val = intVal(input);
        if (unionDom.contains(val)) {
          return false;
        }
        unionDom.emplace(val);
      }
    }
    if (unionDom.size() == inputs.size()) {
      return true;
    }
    return false;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const bool shouldHold = boolVal(reified);
    if (inputs.empty()) {
      return shouldHold;
    }
    std::unordered_set<Int> fixedVals;
    fixedVals.reserve(inputs.size());
    for (const auto& input : inputs) {
      if (isFixed(input)) {
        if (fixedVals.contains(intVal(input))) {
          return !shouldHold;
        }
        fixedVals.emplace(intVal(input));
      }
    }
    for (const auto& input : inputs) {
      if (isFixed(input)) {
        continue;
      }
      const auto& vNode = varNodeConst(input);
      if (vNode.constDomain()->size() > fixedVals.size()) {
        continue;
      }
      const bool noFreeVal =
          std::all_of(vNode.constDomain()->begin(), vNode.constDomain()->end(),
                      [&](Int val) { return fixedVals.contains(val); });
      if (noFreeVal) {
        return !shouldHold;
      }
    }
    const size_t numFree = inputs.size() - fixedVals.size();
    if (numFree <= 1) {
      return shouldHold;
    }
    return !shouldHold;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(
        inputs, [&](const std::string& input) { return !isFixed(input); });
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
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(reified));
  }
};

RC_GTEST_FIXTURE_PROP(fzn_all_different_intTest, RapidCheck, ()) {
  rapidCheck();
}
}  // namespace atlantis::testing
