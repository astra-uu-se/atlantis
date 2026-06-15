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

class fzn_gcc_low_upTest : public fzn_gccTest {
 public:
  std::vector<std::string> outputs{};
  std::vector<std::string> low{};
  std::vector<std::string> up{};
  std::vector<std::pair<size_t, std::string>> lowDuplicates{};
  std::vector<std::pair<size_t, std::string>> upDuplicates{};

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
          lowDuplicates.emplace_back(cover.size(), low.at(j));
          upDuplicates.emplace_back(cover.size(), up.at(j));
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
          coverDuplicates.at(dupIndex).second = covIndex;
          lowDuplicates.at(dupIndex).second = covIndex;
          upDuplicates.at(dupIndex).second = covIndex;
        }
      }
    }
  }

  [[nodiscard]] std::vector<Int> getLow() const {
    std::vector<Int> lowVals(low.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      lowVals.at(i) = intVal(low.at(i));
    }
    for (const auto& [index, identifier] : lowDuplicates) {
      lowVals.at(index) = std::max(lowVals.at(index), intVal(identifier));
    }
    return lowVals;
  }

  [[nodiscard]] std::vector<Int> getUp() const {
    std::vector<Int> upVals(up.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      upVals.at(i) = intVal(up.at(i));
    }
    for (const auto& [index, identifier] : upDuplicates) {
      upVals.at(index) = std::max(upVals.at(index), intVal(identifier));
    }
    return upVals;
  }
};

}  // namespace atlantis::testing
