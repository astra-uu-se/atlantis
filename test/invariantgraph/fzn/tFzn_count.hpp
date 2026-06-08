#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <ranges>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;

class fzn_countTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::string needle{"needle"};
  std::string output{"output"};
  std::string reified{"reified"};

  [[nodiscard]] std::pair<Int, Int> getBounds() const {
    std::unordered_map<Int, std::pair<Int, Int>> bounds;
    if (isFixed(needle)) {
      bounds.reserve(1);
      bounds.emplace(intVal(needle), std::pair<Int, Int>{0, 0});
    } else {
      const auto& needleDom = varNodeConst(needle).constDomain();
      bounds.reserve(needleDom->size());
      for (auto domIter = needleDom->begin(); domIter != needleDom->end(); ++domIter) {
        bounds.emplace(*domIter, std::pair<Int, Int>{0, 0});
      }
    }
    for (const auto& input : inputs) {
      if (isFixed(input)) {
        if (bounds.contains(intVal(input))) {
          ++bounds.at(intVal(input)).first;
          ++bounds.at(intVal(input)).second;
        }
      } else {
        const auto& inputDom = varNodeConst(input).constDomain();
        for (auto domIter = inputDom->begin(); domIter != inputDom->end(); ++domIter) {
          if (bounds.contains(*domIter)) {
            ++bounds.at(*domIter).second;
          }
        }
      }
    }
    Int lb = std::numeric_limits<Int>::max();
    Int ub = std::numeric_limits<Int>::min();
    for (const auto& [l, u] : std::views::values(bounds)) {
      lb = std::min(lb, l);
      ub = std::max(ub, u);
    }

    return {lb, ub};
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

}  // namespace atlantis::testing
