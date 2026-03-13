#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_global_cardinalityTest : public FznTestBase {
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

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "FznCountEqTest::isSatisfied(" << to_string(committedValue)
             << ")" << std::endl;
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
      RC_LOG() << input << " = " << val << std::endl;
      if (valToIndices.contains(val)) {
        for (const size_t index : valToIndices.at(val)) {
          RC_ASSERT(index < cover.size());
          ++counts.at(index);
        }
      }
    }
    std::vector<Int> outs;
    outs.reserve(cover.size());
    for (const auto& o : outputs) {
      const Int oVal = intVal(o, committedValue);
      RC_LOG() << o << " = " << oVal << std::endl;
      outs.emplace_back(oVal);
    }

    bool expected = true;

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
    if (cover.empty()) {
      return isFixedTo(reified, true);
    }
    const auto bounds = getBounds();
    bool alwaysSat = true;
    for (size_t i = 0; alwaysSat && i < bounds.size(); ++i) {
      const auto [lb, ub] = bounds.at(i);
      if (lb == ub) {
        alwaysSat &= isFixedTo(reified, true) ? isFixedTo(outputs.at(i), lb)
                                              : !inDomain(outputs.at(i), lb);
      } else {
        const bool alwaysUnsat =
            ub < lowerBound(outputs.at(i)) || upperBound(outputs.at(i)) < lb;
        alwaysSat &= isFixedTo(reified, false) ? alwaysUnsat : false;
      }
    }
    if (alwaysSat) {
      return alwaysSat;
    }
    if (isFixedTo(reified, false)) {
      for (size_t i = 0; i < cover.size(); ++i) {
        const Int iCover = intVal(cover.at(i));
        for (size_t j = i + 1; j < cover.size(); ++j) {
          const Int jCover = intVal(cover.at(i));
          if (iCover == jCover) {
            if (isFixed(outputs.at(i)) && isFixed(outputs.at(j))) {
              if (intVal(outputs.at(i)) != intVal(outputs.at(j))) {
                return true;
              }
            } else if (isFixed(outputs.at(i))) {
              if (!varNodeConst(outputs.at(j)).inDomain(iCover)) {
                return true;
              }
            } else if (isFixed(outputs.at(j))) {
              if (!varNodeConst(outputs.at(i)).inDomain(jCover)) {
                return true;
              }
            } else {
              const auto& iDom = varNodeConst(outputs.at(i)).constDomain();
              const auto& jDom = varNodeConst(outputs.at(j)).constDomain();
              if (iDom->isDisjoint(*jDom)) {
                return true;
              }
            }
          }
        }
      }
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (cover.empty()) {
      return isFixedTo(reified, false);
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
    if (isFixedTo(reified, true)) {
      for (size_t i = 0; i < cover.size(); ++i) {
        const Int iCover = intVal(cover.at(i));
        for (size_t j = i + 1; j < cover.size(); ++j) {
          const Int jCover = intVal(cover.at(i));
          if (iCover == jCover) {
            if (isFixed(outputs.at(i)) && isFixed(outputs.at(j))) {
              if (intVal(outputs.at(i)) != intVal(outputs.at(j))) {
                return true;
              }
            } else if (isFixed(outputs.at(i))) {
              if (!varNodeConst(outputs.at(j)).inDomain(iCover)) {
                return true;
              }
            } else if (isFixed(outputs.at(j))) {
              if (!varNodeConst(outputs.at(i)).inDomain(jCover)) {
                return true;
              }
            } else {
              const auto& iDom = varNodeConst(outputs.at(i)).constDomain();
              const auto& jDom = varNodeConst(outputs.at(j)).constDomain();
              if (iDom->isDisjoint(*jDom)) {
                return true;
              }
            }
          }
        }
      }
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
    constraintIdentifier =
        isReified ? "fzn_global_cardinality_reif" : "fzn_global_cardinality";
    if (isReified) {
      addBoolArg(BoolArgState::FIXED_FALSE, reified);
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

RC_GTEST_FIXTURE_PROP(fzn_global_cardinalityTest, RapidCheck, ()) {
  rapidCheck(false);
}

}  // namespace atlantis::testing
