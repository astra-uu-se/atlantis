#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "./tFzn_gcc.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

class fzn_gcc_countTest : public fzn_gccTest {
 public:
  std::vector<std::string> outputs{};
  std::vector<std::pair<size_t, std::string>> outputDuplicates{};

  void fixGenerate() override {
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
          outputDuplicates.emplace_back(cover.size(), outputs.at(j));
        }
      }
    }
    for (Int i = static_cast<Int>(cover.size()) - 1; i >= 0; --i) {
      if (!firstOrDistinct.at(i)) {
        cover.erase(cover.begin() + i);
        outputs.erase(outputs.begin() + i);
      }
    }
    for (size_t dupIndex = 0; dupIndex < coverDuplicates.size(); ++dupIndex) {
      const Int dupVal = intVal(coverDuplicates.at(dupIndex).second);
      for (size_t covIndex = 0; covIndex < cover.size(); ++covIndex) {
        if (intVal(cover.at(covIndex)) == dupVal) {
          coverDuplicates.at(dupIndex).first = covIndex;
          outputDuplicates.at(dupIndex).first = covIndex;
        }
      }
    }
  }

  [[nodiscard]] std::vector<std::optional<SearchDomain>> getOutputDomains()
      const {
    std::vector<std::optional<SearchDomain>> domains;
    domains.reserve(outputs.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      domains.emplace_back(
          isFixed(outputs.at(i))
              ? SearchDomain(std::vector<Int>{intVal(outputs.at(i))})
              : SearchDomain(*varNodeConst(outputs.at(i)).constDomain()));
    }
    for (const auto& [index, identifier] : outputDuplicates) {
      if (!domains.at(index).has_value()) {
        continue;
      }
      if (isFixed(identifier)) {
        const Int val = intVal(identifier);
        if (domains.at(index)->contains(val)) {
          domains.at(index)->fix(val);
        } else {
          domains.at(index) = std::nullopt;
        }
      } else if (domains.at(index)->isDisjoint(
                     *varNodeConst(identifier).constDomain())) {
        domains.at(index) = std::nullopt;
      } else {
        domains.at(index)->removeAllValuesExcept(
            *varNodeConst(identifier).constDomain());
      }
    }
    return domains;
  }

  [[nodiscard]] bool duplicateOutputsEquals(const bool committedValue) const {
    return std::ranges::all_of(outputDuplicates, [&](const auto& pair) {
      return intVal(pair.second, committedValue) ==
             intVal(outputs.at(pair.first), committedValue);
    });
  }
};

}  // namespace atlantis::testing
