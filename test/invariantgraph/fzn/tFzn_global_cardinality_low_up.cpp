#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <ranges>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_low_up.hpp"

namespace atlantis::testing {
using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_global_cardinality_low_upTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::vector<std::string> cover{};
  std::vector<std::string> low{};
  std::vector<std::string> up{};
  std::string reified{"reified"};

  [[nodiscard]] std::vector<std::pair<Int, Int>> getBounds() const {
    std::vector<std::pair<Int, Int>> bounds{};
    bounds.reserve(low.size());
    for (const auto& c : cover) {
      const Int needle = intVal(c);
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

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "fzn_global_cardinality_low_up::isSatisfied("
             << to_string(committedValue) << ")" << std::endl;
    std::vector<Int> counts(cover.size(), 0);
    std::unordered_map<Int, std::vector<size_t>> valToIndices;
    valToIndices.reserve(cover.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      const Int needle = intVal(cover.at(i), committedValue);
      if (valToIndices.contains(needle)) {
        valToIndices.at(needle).emplace_back(i);
      } else {
        valToIndices.emplace(needle, std::vector<size_t>{i});
      }
    }
    for (const auto& input : inputs) {
      const Int val = intVal(input);
      if (valToIndices.contains(val)) {
        for (const size_t index : valToIndices.at(val)) {
          RC_ASSERT(index < cover.size());
          ++counts.at(index);
        }
      }
    }
    bool expected = true;
    for (size_t i = 0; i < cover.size(); ++i) {
      const Int lv = intVal(low.at(i), committedValue);
      const Int uv = intVal(up.at(i), committedValue);
      RC_LOG() << cover.at(i) << " = " << counts.at(i) << " [" << lv << " .. "
               << uv << ']' << std::endl;
      expected &= lv <= counts.at(i) && counts.at(i) <= uv;
    }

    const bool actual = boolVal(reified, committedValue);

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
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    return false;
  }

  void generate() override {
    const size_t inputSize = true ? 3 : *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(inputSize);
    for (size_t i = 0; i < inputSize; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }

    const size_t coverSize = true ? 1 : *rc::gen::inRange<size_t>(0, 4);
    cover.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      cover.emplace_back("cover_" + std::to_string(i));
      low.emplace_back("low_" + std::to_string(i));
      up.emplace_back("up_" + std::to_string(i));
    }

    addIntVarArray({IntArgState::FIXED, IntArgState::VAR, IntArgState::PAR}, {{-2, -2}, {-3, 3}, {-3, -3}}, inputs, "inputs");
    addIntVarArray(std::vector(cover.size(), IntArgState::PAR), {{-2, -2}}, cover, "cover");
    addIntVarArray(std::vector(low.size(), IntArgState::PAR), {{-3, -3}}, low, "low");
    addIntVarArray(std::vector(up.size(), IntArgState::PAR), {{2, 2}}, up, "up");

    const bool isReified = true ? false : *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "fzn_global_cardinality_low_up_reif"
                                     : "fzn_global_cardinality_low_up";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputs, [&](const std::string& input) {
      return varId(input) != propagation::NULL_ID;
    });
  }

  void move(bool committedValue) override {
    for (const auto& input : inputs) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    for (const auto& vId : std::array{varId(reified), totalViolationVarId()}) {
      if (vId != propagation::NULL_ID) {
        _solver->query(vId);
      }
    }
  }
};

RC_GTEST_FIXTURE_PROP(fzn_global_cardinality_low_upTest, RapidCheck, ()) {
  rapidCheck(true, true);
}
}  // namespace atlantis::testing