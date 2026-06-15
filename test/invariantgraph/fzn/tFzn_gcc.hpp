#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

class fzn_gccTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::vector<std::string> cover{};
  std::string reified{"reified"};
  std::vector<std::pair<size_t, std::string>> coverDuplicates{};

  virtual void fixGenerate() {
    std::vector<bool> firstOrDistinct(cover.size(), true);
    for (size_t i = 0; i < cover.size(); ++i) {
      if (!firstOrDistinct.at(i)) {
        continue;
      }
      const Int iCov = intVal(cover.at(i));
      for (size_t j = i + 1; j < cover.size(); ++j) {
        if (!firstOrDistinct.at(j)) {
          continue;
        }
        if (iCov == intVal(cover.at(j))) {
          firstOrDistinct.at(j) = false;
          coverDuplicates.emplace_back(cover.size(), cover.at(j));
        }
      }
    }
    for (Int i = static_cast<Int>(cover.size()) - 1; i >= 0; --i) {
      if (!firstOrDistinct.at(i)) {
        cover.erase(cover.begin() + i);
      }
    }
    for (size_t dupIndex = 0; dupIndex < coverDuplicates.size(); ++dupIndex) {
      const Int dupVal = intVal(coverDuplicates.at(dupIndex).second);
      for (size_t covIndex = 0; covIndex < cover.size(); ++covIndex) {
        if (intVal(cover.at(covIndex)) == dupVal) {
          coverDuplicates.at(dupIndex).first = covIndex;
        }
      }
    }
  }

  [[nodiscard]] std::vector<std::pair<Int, Int>> getBounds() const {
    std::vector<std::pair<Int, Int>> bounds{};
    bounds.reserve(cover.size());
    for (const std::string& covIdentifier : cover) {
      const Int needle = intVal(covIdentifier);
      Int lb = 0;
      Int ub = 0;
      for (const auto& input : inputs) {
        if (isFixed(input)) {
          if (intVal(input) == needle) {
            ++lb;
            ++ub;
          }
        } else {
          if (inDomain(input, needle)) {
            ++ub;
          }
        }
      }
      bounds.emplace_back(lb, ub);
    }
    return bounds;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputs, [&](const std::string& input) {
      return varId(input) != propagation::NULL_ID;
    });
  }

  void move(const bool committedValue) override {
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
};

}  // namespace atlantis::testing
