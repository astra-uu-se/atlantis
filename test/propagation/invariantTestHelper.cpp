#include "./invariantTestHelper.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::propagation;


inline VarViewId InvariantTest::makeIntVar(
    const Int lb, const Int ub, std::uniform_int_distribution<Int>& dist) {
  const Int val = generateState == GenerateState::RANDOM
                      ? dist(gen)
                      : (generateState == GenerateState::LB ? lb : ub);
  return _solver->makeIntVar(val, lb, ub);
}
std::vector<Int> InvariantTest::createInputVals(
    const std::vector<VarViewId>& inputVars) const {
  std::vector<Int> inputVals(inputVars.size());
  for (size_t i = 0; i < inputVars.size(); ++i) {
    inputVals.at(i) = _solver->lowerBound(inputVars.at(i));
  }
  return inputVals;
}
size_t InvariantTest::trySetNextInputVarVal(
    const std::vector<VarViewId>& inputVars,
    std::vector<Int>& inputVals) const {
  EXPECT_EQ(inputVars.size(), inputVals.size());
  for (size_t i = 0; i < inputVars.size(); ++i) {
    if (inputVals.at(i) < _solver->upperBound(inputVars.at(i))) {
      inputVals.at(i) += 1;
      _solver->setValue(inputVars.at(i), inputVals.at(i));
      return i;
    }
    EXPECT_EQ(inputVals.at(i), _solver->upperBound(inputVars.at(i)));
    inputVals.at(i) = _solver->lowerBound(inputVars.at(i));
  }
  return inputVars.size();
}
size_t InvariantTest::trySetMinDiffInputVarVal(
    const std::vector<VarViewId>& inputVars,
    std::vector<Int>& inputVals) const {
  EXPECT_EQ(inputVars.size(), inputVals.size());
  Int minDiff = std::numeric_limits<Int>::max();
  for (size_t i = 0; i < inputVars.size(); ++i) {
    EXPECT_LE(_solver->lowerBound(inputVars.at(i)), inputVals.at(i));
    EXPECT_LE(inputVals.at(i), _solver->upperBound(inputVars.at(i)));
    if (inputVals.at(i) < _solver->upperBound(inputVars.at(i))) {
      minDiff = std::min(
          minDiff, inputVals.at(i) - _solver->lowerBound(inputVars.at(i)));
    }
  }
  for (size_t i = 0; i < inputVars.size(); ++i) {
    const Int diff = inputVals.at(i) - _solver->lowerBound(inputVars.at(i));
    if (diff == minDiff &&
        inputVals.at(i) < _solver->upperBound(inputVars.at(i))) {
      inputVals.at(i) += 1;
      _solver->setValue(inputVars.at(i), inputVals.at(i));
      return i;
    }
  }
  return inputVars.size();
}
std::vector<Int> InvariantTest::makeValVector(
    const std::vector<std::pair<Int, Int>>& bounds) {
  std::vector<Int> vals(bounds.size());
  for (size_t i = 0; i < bounds.size(); ++i) {
    vals.at(i) = bounds.at(i).first;
  }
  return vals;
}
std::vector<VarViewId> InvariantTest::makeVars(
    const std::vector<Int>& vals,
    const std::vector<std::pair<Int, Int>>& bounds) {
  std::vector<VarViewId> vars;
  EXPECT_EQ(vals.size(), bounds.size());
  vars.reserve(vals.size());
  for (size_t i = 0; i < vals.size(); ++i) {
    EXPECT_GE(vals.at(i), bounds.at(i).first);
    EXPECT_LE(vals.at(i), bounds.at(i).second);
    vars.emplace_back(_solver->makeIntVar(vals.at(i), bounds.at(i).first,
                                          bounds.at(i).second));
  }
  return vars;
}

inline std::vector<VarViewId> InvariantTest::makeVars(const size_t numVars, const Int lb,
                                                      const Int ub) {
  EXPECT_LE(lb, ub);
  std::vector<VarViewId> vars;
  vars.reserve(numVars);
  for (size_t i = 0; i < numVars; ++i) {
    vars.emplace_back(_solver->makeIntVar(lb, lb, ub));
  }
  return vars;
}
std::vector<VarViewId> InvariantTest::makeVars(
    const std::vector<std::pair<Int, Int>>& bounds) {
  std::vector<VarViewId> vars;
  vars.reserve(bounds.size());
  for (const auto& [lb, ub] : bounds) {
    EXPECT_LE(lb, ub);
    vars.emplace_back(_solver->makeIntVar(lb, lb, ub));
  }
  return vars;
}
VarViewId InvariantTest::getMinVarViewId(const std::vector<VarViewId>& vars) {
  return *std::ranges::min_element(vars.begin(), vars.end(),
                                   [&](const VarViewId& a, const VarViewId& b) {
                                     return size_t{a} < size_t{b};
                                   });
}
VarViewId InvariantTest::getMaxVarViewId(const std::vector<VarViewId>& vars) {
  return *std::ranges::max_element(vars.begin(), vars.end(),
                                   [&](const VarViewId& a, const VarViewId& b) {
                                     return size_t{a} < size_t{b};
                                   });
}
void InvariantTest::expectNextInput(const std::vector<VarViewId>& inputVars,
                                    Invariant& invariant) const {
  for (const auto& id : inputVars) {
    EXPECT_TRUE(id.isVar());
  }
  const auto minVarId = size_t{getMinVarViewId(inputVars)};
  const auto maxVarId = size_t{getMaxVarViewId(inputVars)};

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId - minVarId + 1, false);
    for (size_t i = 0; i < inputVars.size(); ++i) {
      const size_t varId = size_t{invariant.nextInput(ts)};
      EXPECT_NE(varId, NULL_ID);
      EXPECT_LE(minVarId, varId);
      EXPECT_GE(maxVarId, varId);
      EXPECT_FALSE(notified.at(varId - minVarId));
      notified.at(varId - minVarId) = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    for (size_t i = 0; i <= maxVarId - minVarId; ++i) {
      EXPECT_TRUE(notified.at(i));
    }
  }
}
Int InvariantTest::increaseNextVal(
    const std::vector<std::pair<Int, Int>>& bounds, std::vector<Int>& vals) {
  EXPECT_EQ(bounds.size(), vals.size());
  for (Int i = static_cast<Int>(vals.size()) - 1; i >= 0; --i) {
    EXPECT_GE(vals.at(i), bounds.at(i).first);
    EXPECT_LE(vals.at(i), bounds.at(i).second);
    if (vals.at(i) < bounds.at(i).second) {
      ++vals.at(i);
      return i;
    }
    vals.at(i) = bounds.at(i).first;
  }
  return -1;
}
Int InvariantTest::increaseNextVal(const std::vector<VarViewId>& varIds,
                                   std::vector<Int>& inputVals) const {
  EXPECT_EQ(varIds.size(), inputVals.size());
  for (Int i = static_cast<Int>(inputVals.size() - 1); i >= 0; --i) {
    if (varIds.at(i) == NULL_ID) {
      continue;
    }
    if (inputVals.at(i) < _solver->upperBound(varIds.at(i))) {
      ++inputVals.at(i);
      return i;
    }
    inputVals.at(i) = _solver->lowerBound(varIds.at(i));
  }
  return -1;
}

inline void InvariantTest::setVarVals(const Timestamp ts,
                                      const std::vector<VarViewId>& inputVars,
                                      const std::vector<Int>& vals) {
  EXPECT_EQ(inputVars.size(), vals.size());
  for (size_t i = 0; i < inputVars.size(); ++i) {
    if (inputVars.at(i) != NULL_ID) {
      _solver->setValue(ts, inputVars.at(i), vals.at(i));
    }
  }
}
void InvariantTest::setVarVals(const std::vector<VarViewId>& inputVars,
                               const std::vector<Int>& vals) {
  EXPECT_EQ(inputVars.size(), vals.size());
  for (size_t i = 0; i < inputVars.size(); ++i) {
    if (inputVars.at(i) != NULL_ID) {
      _solver->setValue(inputVars.at(i), vals.at(i));
    }
  }
}
void InvariantTest::notifyInputsChanged(
    const Timestamp ts, Invariant& invariant,
    const std::vector<VarViewId>& inputVars) const {
  for (LocalId i = 0; i < inputVars.size(); ++i) {
    if (_solver->value(ts, inputVars.at(i)) !=
        _solver->committedValue(inputVars.at(i))) {
      invariant.notifyInputChanged(ts, i);
    }
  }
}

inline std::pair<Int, Int> InvariantTest::genBounds(const Int lb, const Int ub) {
  return *rc::gen::suchThat(
      rc::gen::pair<Int, Int>(rc::gen::inRange<Int>(lb, ub),
                              rc::gen::inRange<Int>(lb, ub)),
      [](const std::pair<Int, Int>& p) { return p.first <= p.second; });
}

}