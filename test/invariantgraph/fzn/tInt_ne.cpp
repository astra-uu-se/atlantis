#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_ne.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_neTest : public FznTestBase {
 public:
  std::string a{"i_1"};
  std::string b{"i_2"};
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        intVal(a, committedValue) != intVal(b, committedValue);
    const bool actual = boolVal(reified, committedValue);

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  void generate() override {
    addIntArg(a);
    addIntArg(b);
    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "int_ne_reif" : "int_ne";
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
    if (isFixed(a) && isFixed(b)) {
      return boolVal(reified) ? intVal(a) != intVal(b) : intVal(a) == intVal(b);
    }
    if (isFixed(a)) {
      return boolVal(reified) ? !inDomain(b, intVal(a))
                              : inDomain(b, intVal(a));
    }
    if (isFixed(b)) {
      return boolVal(reified) ? !inDomain(a, intVal(b))
                              : inDomain(a, intVal(b));
    }
    const auto& aDom = varNodeConst(a).constDomain();
    const auto& bDom = varNodeConst(b).constDomain();
    return boolVal(reified) ? aDom->isDisjoint(*bDom) : false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (isFixed(a) && isFixed(b)) {
      return boolVal(reified) ? intVal(a) == intVal(b) : intVal(a) != intVal(b);
    }
    if (isFixed(a)) {
      return boolVal(reified) ? inDomain(b, intVal(a))
                              : !inDomain(b, intVal(a));
    }
    if (isFixed(b)) {
      return boolVal(reified) ? inDomain(a, intVal(b))
                              : !inDomain(a, intVal(b));
    }
    const auto& aDom = varNodeConst(a).constDomain();
    const auto& bDom = varNodeConst(b).constDomain();
    return boolVal(reified) ? false : aDom->isDisjoint(*bDom);
  }

  [[nodiscard]] bool canMove() const override {
    return varId(a) != propagation::NULL_ID || varId(b) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    std::unordered_set<InvariantNodeId, InvariantNodeIdHash>
        implicitConstraints;
    std::vector<bool> hasImplicitConstraints(2, false);
    implicitConstraints.reserve(2);
    for (size_t i = 0; i < 2; ++i) {
      const auto& input = i == 0 ? a : b;
      if (!isFixed(input)) {
        const auto& defNodes = varNodeConst(input).definingNodes();
        if (!defNodes.empty()) {
          RC_ASSERT(defNodes.size() == size_t{1});
          const InvariantNodeId implId = *defNodes.begin();
          RC_ASSERT(implId.isImplicitConstraint());
          implicitConstraints.emplace(implId);
          hasImplicitConstraints.at(i) = true;
        }
      }
    }

    for (size_t i = 0; i < 2; ++i) {
      const auto& input = i == 0 ? a : b;
      if (!hasImplicitConstraints.at(i) && !isFixed(input) && randBool()) {
        changeValue(input, committedValue);
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

RC_GTEST_FIXTURE_PROP(int_neTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing