#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <deque>
#include <random>
#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::propagation;

// below function returns the subsets of vector origin.
template <class T>
std::vector<std::vector<T>> subsets(std::vector<T>& origin) {
  std::vector<T> subset;
  std::vector<std::vector<T>> res;
  std::deque<std::pair<size_t, size_t>> q;
  res.emplace_back(std::vector<T>{});
  q.emplace_back(size_t(0), 0 + 1);
  q.emplace_back(std::pair<size_t, size_t>{res.size(), 0 + 1});
  res.emplace_back(std::vector<T>{origin.at(0)});

  while (!q.empty()) {
    const auto& [resIndex, originIndex] = q.front();
    q.pop_front();
    for (size_t j = originIndex; j < origin.size(); ++j) {
      q.emplace_back(std::pair<size_t, size_t>{res.size(), j + 1});
      res.emplace_back(res.at(resIndex));
      res.back().emplace_back(origin.at(j));
    }
  }
  return res;
}

template <class T, class U>
std::vector<std::pair<T, U>> cartesianProduct(const std::vector<T>& t,
                                              const std::vector<U>& u) {
  std::vector<std::pair<T, U>> prod(t.size() * u.size());
  for (size_t i = 0; i < t.size(); ++i) {
    for (size_t j = 0; j < u.size(); ++j) {
      prod.at(i * u.size() + j) = std::pair<T, U>{t.at(i), u.at(j)};
    }
  }
  return prod;
}

template <class T>
std::vector<std::pair<T, T>> cartesianProduct(const std::vector<T>& t) {
  return cartesianProduct(t, t);
}

struct NotificationData {
  PropagationMode propMode{PropagationMode::INPUT_TO_OUTPUT};
  OutputToInputMarkingMode markingMode{OutputToInputMarkingMode::NONE};
  size_t numNextInputCalls{0};
  VarViewId modifiedVarId{NULL_ID};
  Int modifiedVal{0};
  VarViewId queryVarId{NULL_ID};
};

enum struct GenerateState : uint8_t { RANDOM, LB, UB };

class InvariantTest : public ::testing::Test {
  std::uniform_int_distribution<unsigned char> binaryDist;

 protected:
  GenerateState generateState{GenerateState::RANDOM};
  std::shared_ptr<Solver> _solver;
  std::mt19937 gen;
  std::default_random_engine rng;
  std::vector<std::pair<PropagationMode, OutputToInputMarkingMode>>
      propMarkModes{
          {PropagationMode::INPUT_TO_OUTPUT, OutputToInputMarkingMode::NONE},
          {PropagationMode::OUTPUT_TO_INPUT, OutputToInputMarkingMode::NONE},
          {PropagationMode::OUTPUT_TO_INPUT,
           OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC},
          {PropagationMode::OUTPUT_TO_INPUT,
           OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION}};

  VarViewId makeIntVar(Int lb, Int ub,
                       std::uniform_int_distribution<Int>& dist) {
    const Int val = generateState == GenerateState::RANDOM
                        ? dist(gen)
                        : (generateState == GenerateState::LB ? lb : ub);
    return _solver->makeIntVar(val, lb, ub);
  }

  [[nodiscard]] std::vector<Int> createInputVals(
      const std::vector<VarViewId>& inputVars) const {
    std::vector<Int> inputVals(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      inputVals.at(i) = _solver->lowerBound(inputVars.at(i));
    }
    return inputVals;
  }

  size_t trySetNextInputVarVal(const std::vector<VarViewId>& inputVars,
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

  size_t trySetMinDiffInputVarVal(const std::vector<VarViewId>& inputVars,
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

  static std::vector<Int> makeValVector(
      const std::vector<std::pair<Int, Int>>& bounds) {
    std::vector<Int> vals(bounds.size());
    for (size_t i = 0; i < bounds.size(); ++i) {
      vals.at(i) = bounds.at(i).first;
    }
    return vals;
  }

  std::vector<VarViewId> makeVars(
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

  std::vector<VarViewId> makeVars(size_t numVars, Int lb, Int ub) {
    EXPECT_LE(lb, ub);
    std::vector<VarViewId> vars;
    vars.reserve(numVars);
    for (size_t i = 0; i < numVars; ++i) {
      vars.emplace_back(_solver->makeIntVar(lb, lb, ub));
    }
    return vars;
  }

  std::vector<VarViewId> makeVars(
      const std::vector<std::pair<Int, Int>>& bounds) {
    std::vector<VarViewId> vars;
    vars.reserve(bounds.size());
    for (const auto& [lb, ub] : bounds) {
      EXPECT_LE(lb, ub);
      vars.emplace_back(_solver->makeIntVar(lb, lb, ub));
    }
    return vars;
  }

  std::vector<Int> makeValVector(const std::vector<VarViewId>& inputVars) {
    std::vector<Int> vals(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      EXPECT_NE(inputVars.at(i), NULL_ID);
      vals.at(i) = _solver->lowerBound(inputVars.at(i));
    }
    return vals;
  }

  [[nodiscard]] static VarViewId getMinVarViewId(
      const std::vector<VarViewId>& vars) {
    return *std::ranges::min_element(
        vars.begin(), vars.end(), [&](const VarViewId& a, const VarViewId& b) {
          return size_t(a) < size_t(b);
        });
  }

  [[nodiscard]] static VarViewId getMaxVarViewId(
      const std::vector<VarViewId>& vars) {
    return *std::ranges::max_element(
        vars.begin(), vars.end(), [&](const VarViewId& a, const VarViewId& b) {
          return size_t(a) < size_t(b);
        });
  }

  void expectNextInput(const std::vector<VarViewId>& inputVars,
                       Invariant& invariant) const {
    for (const auto& id : inputVars) {
      EXPECT_TRUE(id.isVar());
    }
    const auto minVarId = size_t(getMinVarViewId(inputVars));
    const auto maxVarId = size_t(getMaxVarViewId(inputVars));

    for (Timestamp ts = _solver->currentTimestamp() + 1;
         ts < _solver->currentTimestamp() + 4; ++ts) {
      std::vector<bool> notified(maxVarId - minVarId + 1, false);
      for (size_t i = 0; i < inputVars.size(); ++i) {
        const size_t varId = size_t(invariant.nextInput(ts));
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

  static Int increaseNextVal(const std::vector<std::pair<Int, Int>>& bounds,
                             std::vector<Int>& vals) {
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

  Int increaseNextVal(const std::vector<VarViewId>& varIds,
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

  void setVarVals(Timestamp ts, const std::vector<VarViewId>& inputVars,
                  const std::vector<Int>& vals) {
    EXPECT_EQ(inputVars.size(), vals.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      if (inputVars.at(i) != NULL_ID) {
        _solver->setValue(ts, inputVars.at(i), vals.at(i));
      }
    }
  }

  void setVarVals(const std::vector<VarViewId>& inputVars,
                  const std::vector<Int>& vals) {
    EXPECT_EQ(inputVars.size(), vals.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      if (inputVars.at(i) != NULL_ID) {
        _solver->setValue(inputVars.at(i), vals.at(i));
      }
    }
  }

  void notifyInputsChanged(Timestamp ts, Invariant& invariant,
                           const std::vector<VarViewId>& inputVars) const {
    for (LocalId i = 0; i < inputVars.size(); ++i) {
      if (_solver->value(ts, inputVars.at(i)) !=
          _solver->committedValue(inputVars.at(i))) {
        invariant.notifyInputChanged(ts, i);
      }
    }
  }

  template <class T>
  void testNotifications(T* invariant, NotificationData data) {
    EXPECT_CALL(*invariant, recompute(::testing::_)).Times(AtLeast(1));
    EXPECT_CALL(*invariant, commit(::testing::_)).Times(AtLeast(1));

    if (!_solver->isOpen()) {
      _solver->open();
    }
    _solver->setPropagationMode(data.propMode);
    _solver->setOutputToInputMarkingMode(data.markingMode);
    _solver->close();

    if (_solver->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, nextInput(::testing::_)).Times(0);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(::testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant, notifyInputChanged(::testing::_, ::testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(*invariant, nextInput(::testing::_))
          .Times(data.numNextInputCalls);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(::testing::_)).Times(1);

      EXPECT_CALL(*invariant, notifyInputChanged(::testing::_, ::testing::_))
          .Times(AtMost(1));
    }

    _solver->beginMove();
    _solver->setValue(data.modifiedVarId, data.modifiedVal);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(data.queryVarId);
    _solver->endProbe();
    _solver->open();
  }

  bool randBool() { return binaryDist(gen) == 1; }

  static std::pair<Int, Int> genBounds(Int lb, Int ub) {
    return *rc::gen::suchThat(
        rc::gen::pair<Int, Int>(rc::gen::inRange<Int>(lb, ub),
                                rc::gen::inRange<Int>(lb, ub)),
        [](const std::pair<Int, Int>& p) { return p.first <= p.second; });
  }

  static std::pair<Int, Int> genBounds() {
    return genBounds(std::numeric_limits<Int>::min(),
                     std::numeric_limits<Int>::max());
  }

 public:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    _solver = std::make_unique<Solver>();
    generateState = GenerateState::RANDOM;
    binaryDist = std::uniform_int_distribution<unsigned char>(0, 1);
  }
};

}  // namespace atlantis::testing
