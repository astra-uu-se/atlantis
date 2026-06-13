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

  [[nodiscard]] std::vector<Int> getCover() const {
    std::vector<bool> firstOrDistinct(cover.size(), true);
    size_t numDistinct = 0;
    for (size_t i = 0; i < cover.size(); ++i) {
      if (!firstOrDistinct.at(i)) {
        continue;
      }
      ++numDistinct;
      const Int iCov = intVal(cover.at(i));
      for (size_t j = i + 1; j < cover.size(); ++j) {
        firstOrDistinct.at(j) = firstOrDistinct.at(j) && iCov != intVal(cover.at(j));
      }
    }
    std::vector<Int> c(numDistinct);
    size_t index = 0;
    for (size_t i = 0; i < cover.size(); ++i) {
      if (firstOrDistinct.at(i)) {
        c[index++] = intVal(cover.at(i));
      }
    }
    return c;
  }

  [[nodiscard]] std::vector<SearchDomain> getCountDomains(const std::vector<std::string>& counts) const {
    std::vector<SearchDomain> domains;
    std::vector<bool> firstOrDistinct(counts.size(), true);
    domains.reserve(cover.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      if (!firstOrDistinct.at(i)) {
        continue;
      }
      SearchDomain iDom = isFixed(counts.at(i)) ? SearchDomain(std::vector<Int>(intVal(counts.at(i)))) : SearchDomain(*varNodeConst(counts.at(i)).constDomain());
      const Int iCov = intVal(cover.at(i));
      for (size_t j = i + 1; j < cover.size(); ++j) {
        if (iCov == intVal(cover.at(j))) {
          firstOrDistinct.at(j) = false;
        }
        if (isFixed(counts.at(j))) {
          iDom.fix(lowerBound(counts.at(j)));
        } else {
          iDom.removeAllValuesExcept(*varNodeConst(counts.at(j)).constDomain());
        }
      }
      domains.emplace_back(iDom);
    }
    return domains;
  }

  [[nodiscard]] std::vector<std::pair<Int, Int>> getLowUp(const std::vector<std::string>& low, const std::vector<std::string>& up) const {
    std::vector<std::pair<Int, Int>> lowup;
    std::vector<bool> firstOrDistinct(lowup.size(), true);
    lowup.reserve(cover.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      if (!firstOrDistinct.at(i)) {
        continue;
      }
      std::pair<Int, Int> lu{intVal(low.at(i)), intVal(up.at(i))};
      const Int iCov = intVal(cover.at(i));
      for (size_t j = i + 1; j < cover.size(); ++j) {
        if (iCov == intVal(cover.at(j))) {
          firstOrDistinct.at(j) = false;
        }
        lu.first = std::max(lu.first, intVal(low.at(j)));
        lu.second = std::min(lu.second, intVal(up.at(j)));
      }
      lowup.emplace_back(lu);
    }
    return lowup;
  }

  [[nodiscard]] std::vector<std::pair<Int, Int>> getBounds(std::vector<Int> cov) const {
    std::vector<std::pair<Int, Int>> bounds{};
    bounds.reserve(cover.size());
    for (const Int needle : cov) {
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
