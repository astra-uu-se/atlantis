#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_closed.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_global_cardinality_closedTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::vector<std::string> cover{};
  std::vector<std::string> outputs{};
  std::string reified{"reified"};

  [[nodiscard]] std::vector<std::pair<Int, Int>> getBounds() const {
    std::vector<std::pair<Int, Int>> bounds{};
    bounds.reserve(cover.size());
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

  std::unordered_map<Int, std::vector<size_t>> valToIndices(
      bool committedValue) const {
    std::unordered_map<Int, std::vector<size_t>> vti;
    vti.reserve(cover.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      const Int needle = intVal(cover.at(i), committedValue);
      if (vti.contains(needle)) {
        vti.at(needle).emplace_back(i);
      } else {
        vti.emplace(needle, std::vector<size_t>{i});
      }
    }
    return vti;
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "FznCountEqTest::isSatisfied(" << to_string(committedValue)
             << ")" << std::endl;
    std::vector<Int> counts(cover.size(), 0);
    const auto vti = valToIndices(committedValue);

    bool expected = true;

    for (const auto& input : inputs) {
      const Int val = intVal(input);
      RC_LOG() << input << " = " << val << std::endl;
      if (vti.contains(val)) {
        for (const size_t index : vti.at(val)) {
          RC_ASSERT(index < cover.size());
          ++counts.at(index);
        }
      } else {
        expected = false;
      }
    }
    std::vector<Int> outs;
    outs.reserve(cover.size());
    for (const auto& o : outputs) {
      const Int oVal = intVal(o, committedValue);
      RC_LOG() << o << " = " << oVal << std::endl;
      outs.emplace_back(oVal);
    }

    for (size_t i = 0; i < cover.size(); ++i) {
      RC_LOG() << "counts[" << i << "] = " << counts.at(i) << std::endl;
      expected &= counts.at(i) == outs.at(i);
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
    if (!isFixed(reified)) {
      return false;
    }
    if (inputs.empty() && cover.empty()) {
      return isFixedTo(reified, true);
    }
    if (cover.empty()) {
      return isFixedTo(reified, false);
    }
    if (inputs.empty()) {
      const bool alwaysUnsat =
          std::ranges::any_of(outputs, [&](const std::string& identifier) {
            return !inDomain(identifier, Int{0});
          });
      if (alwaysUnsat) {
        return isFixedTo(reified, false);
      }
      const bool alwaysSat =
          std::ranges::all_of(outputs, [&](const std::string& identifier) {
            return isFixedTo(identifier, Int{0});
          });
      if (alwaysSat) {
        return isFixedTo(reified, true);
      }
    }

    for (size_t i = 0; i < cover.size(); ++i) {
      for (size_t j = i + 1; j < cover.size(); ++j) {
        if (intVal(cover[i]) == intVal(cover[j])) {
          const auto& iDom = varNodeConst(outputs.at(i)).constDomain();
          const auto& jDom = varNodeConst(outputs.at(i)).constDomain();
          if (iDom->isDisjoint(*jDom)) {
            return isFixedTo(reified, false);
          }
        }
      }
    }

    if (isFixedTo(reified, false)) {
      const auto vti = valToIndices(true);
      for (const auto& input : inputs) {
        if (isFixed(input)) {
          if (!vti.contains(intVal(input))) {
            return true;
          }
        } else {
          const auto& dom = varNodeConst(input).constDomain();
          const bool noOverlap =
              std::none_of(dom->begin(), dom->end(),
                           [&](const Int v) { return vti.contains(v); });
          if (noOverlap) {
            return true;
          }
        }
      }
    }

    const auto bounds = getBounds();
    bool alwaysSat = true;
    for (size_t i = 0; alwaysSat && i < bounds.size(); ++i) {
      const auto [lb, ub] = bounds.at(i);
      if (lb == ub) {
        const bool alwaysUnsat = !inDomain(outputs.at(i), lb);
        if (isFixedTo(reified, false)) {
          if (alwaysUnsat) {
            return true;
          }
        } else {
          alwaysSat &= !alwaysUnsat;
        }
      } else {
        const bool alwaysUnsat =
            ub < lowerBound(outputs.at(i)) || upperBound(outputs.at(i)) < lb;
        if (isFixedTo(reified, false)) {
          if (alwaysUnsat) {
            return true;
          }
        } else {
          alwaysSat &= !alwaysUnsat;
        }
      }
    }
    if (alwaysSat) {
      return alwaysSat;
    }
    std::unordered_set<Int> visitedCovers;
    visitedCovers.reserve(cover.size());
    Int totalLb = 0;
    Int totalUb = 0;
    for (size_t i = 0; i < cover.size(); ++i) {
      if (visitedCovers.contains(intVal(cover.at(i)))) {
        continue;
      }
      visitedCovers.emplace(intVal(cover.at(i)));
      const Int iCover = intVal(cover.at(i));
      SearchDomain combinedDomain =
          isFixed(outputs.at(i))
              ? SearchDomain(intVal(outputs.at(i)), intVal(outputs.at(i)))
              : SearchDomain(*varNodeConst(outputs.at(i)).constDomain());
      for (size_t j = i + 1; j < cover.size(); ++j) {
        const Int jCover = intVal(cover.at(i));
        if (iCover == jCover) {
          if (isFixed(outputs.at(j))) {
            combinedDomain.remove(intVal(outputs.at(j)));
          } else {
            combinedDomain.removeAllValuesExcept(
                *varNodeConst(outputs.at(j)).constDomain());
          }
        }
      }
      if (combinedDomain.size() == 0) {
        return isFixedTo(reified, false);
      }
      totalLb += combinedDomain.lowerBound();
      totalUb += combinedDomain.upperBound();
    }
    if (static_cast<Int>(inputs.size()) < totalLb ||
        totalUb < static_cast<Int>(inputs.size())) {
      return isFixedTo(reified, true);
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (inputs.empty() && cover.empty()) {
      return isFixedTo(reified, false);
    }
    if (cover.empty()) {
      return isFixedTo(reified, true);
    }
    if (inputs.empty()) {
      const bool alwaysUnsat =
          std::ranges::any_of(outputs, [&](const std::string& identifier) {
            return !inDomain(identifier, Int{0});
          });
      if (alwaysUnsat) {
        return isFixedTo(reified, true);
      }
      const bool alwaysSat =
          std::ranges::all_of(outputs, [&](const std::string& identifier) {
            return isFixedTo(identifier, Int{0});
          });
      if (alwaysSat) {
        return isFixedTo(reified, false);
      }
    }

    if (isFixedTo(reified, true)) {
      const auto vti = valToIndices(true);
      for (const auto& input : inputs) {
        if (isFixed(input)) {
          if (!vti.contains(intVal(input))) {
            return true;
          }
        } else {
          const auto& dom = varNodeConst(input).constDomain();
          const bool noOverlap =
              std::none_of(dom->begin(), dom->end(),
                           [&](const Int v) { return vti.contains(v); });
          if (noOverlap) {
            return true;
          }
        }
      }
    }

    const auto bounds = getBounds();
    for (size_t i = 0; i < bounds.size(); ++i) {
      const auto [lb, ub] = bounds.at(i);
      if (lb == ub) {
        const bool neverSat = isFixedTo(reified, false)
                                  ? isFixedTo(outputs.at(i), lb)
                                  : !inDomain(outputs.at(i), lb);
        if (neverSat) {
          return true;
        }
      } else {
        const bool alwaysUnsat =
            ub < lowerBound(outputs.at(i)) || upperBound(outputs.at(i)) < lb;
        const bool neverSat = isFixedTo(reified, true) ? alwaysUnsat : false;
        if (neverSat) {
          return true;
        }
      }
    }
    std::unordered_set<Int> visitedCovers;
    visitedCovers.reserve(cover.size());
    Int totalLb = 0;
    Int totalUb = 0;
    for (size_t i = 0; i < cover.size(); ++i) {
      if (visitedCovers.contains(intVal(cover.at(i)))) {
        continue;
      }
      visitedCovers.emplace(intVal(cover.at(i)));
      const Int iCover = intVal(cover.at(i));
      SearchDomain combinedDomain =
          isFixed(outputs.at(i))
              ? SearchDomain(intVal(outputs.at(i)), intVal(outputs.at(i)))
              : SearchDomain(*varNodeConst(outputs.at(i)).constDomain());
      for (size_t j = i + 1; j < cover.size(); ++j) {
        const Int jCover = intVal(cover.at(i));
        if (iCover == jCover) {
          if (isFixed(outputs.at(j))) {
            combinedDomain.remove(intVal(outputs.at(j)));
          } else {
            combinedDomain.removeAllValuesExcept(
                *varNodeConst(outputs.at(j)).constDomain());
          }
        }
      }
      if (combinedDomain.size() == 0) {
        return isFixedTo(reified, false);
      }
      totalLb += combinedDomain.lowerBound();
      totalUb += combinedDomain.upperBound();
    }
    if (static_cast<Int>(inputs.size()) < totalLb ||
        totalUb < static_cast<Int>(inputs.size())) {
      return isFixedTo(reified, true);
    }

    return false;
  }

  void generate() override {
    const size_t inputSize = *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(inputSize);
    for (size_t i = 0; i < inputSize; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }

    const size_t coverSize = *rc::gen::inRange<size_t>(0, 4);
    cover.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      cover.emplace_back("cover_" + std::to_string(i));
    }

    outputs.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      outputs.emplace_back("output_" + std::to_string(i));
    }

    addIntVarArray(inputs, "inputs");
    addIntVarArray(std::vector(cover.size(), IntArgState::PAR), cover, "cover");
    addIntVarArray(outputs, "outputs");

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "fzn_global_cardinality_closed_reif"
                                     : "fzn_global_cardinality_closed";
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
    for (const auto& output : outputs) {
      if (varId(output) != propagation::NULL_ID) {
        _solver->query(varId(output));
      }
    }
    for (const auto& vId : std::array{varId(reified), totalViolationVarId()}) {
      if (vId != propagation::NULL_ID) {
        _solver->query(vId);
      }
    }
  }
};

RC_GTEST_FIXTURE_PROP(fzn_global_cardinality_closedTest, RapidCheck, ()) {
  rapidCheck(false);
}

}  // namespace atlantis::testing
