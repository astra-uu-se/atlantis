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
    Int numVars = true ? 2 : *rc::gen::inRange<Int>(0, 10);
    const bool isReified = false && *rc::gen::arbitrary<bool>();
    constraintIdentifier =
        isReified ? "fzn_all_different_int_reif" : "fzn_all_different_int";

    for (Int i = 0; i < numVars; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);
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

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (inputs.size() <= 1) {
      return !boolVal(reified);
    }
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
    for (size_t j = 0; j < inputs.size(); ++j) {
      for (size_t k = j + 1; k < inputs.size(); k++) {
        if (domains.at(k).size() == 1) {
          domains.at(j).erase(*domains.at(k).begin());
        }
      }
      if (domains.at(j).empty()) {
        return boolVal(reified);
      }
      if (domains.at(j).size() > 1) {
        continue;
      }
      const Int val = *domains.at(j).begin();
      for (size_t i = 0; i < j; i++) {
        domains.at(i).erase(val);
        if (domains.at(i).empty()) {
          return boolVal(reified);
        }
      }
      for (size_t k = j + 1; k < inputs.size(); k++) {
        domains.at(k).erase(val);
        if (domains.at(k).empty()) {
          return boolVal(reified);
        }
      }
    }
    return false;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (inputs.empty()) {
      return boolVal(reified);
    }
    const size_t numFree = std::ranges::count_if(
        inputs, [&](const auto& var) { return !isFixed(var); });
    if (numFree <= 1) {
      return boolVal(reified);
    }
    return !boolVal(reified);
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
      auto implNode =
          _invariantGraph->implicitConstraintNode(implId).neighborhood();
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
