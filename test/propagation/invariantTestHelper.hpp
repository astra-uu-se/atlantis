#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <deque>
#include <random>
#include <ranges>
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
  q.emplace_back(0, 0 + 1);
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
                       std::uniform_int_distribution<Int>& dist);

  [[nodiscard]] std::vector<Int> createInputVals(
      const std::vector<VarViewId>& inputVars) const;

  size_t trySetNextInputVarVal(const std::vector<VarViewId>& inputVars,
                               std::vector<Int>& inputVals) const;

  size_t trySetMinDiffInputVarVal(const std::vector<VarViewId>& inputVars,
                                  std::vector<Int>& inputVals) const;

  static std::vector<Int> makeValVector(
      const std::vector<std::pair<Int, Int>>& bounds);

  std::vector<VarViewId> makeVars(
      const std::vector<Int>& vals,
      const std::vector<std::pair<Int, Int>>& bounds);

  std::vector<VarViewId> makeVars(size_t numVars, Int lb, Int ub);

  std::vector<VarViewId> makeVars(
      const std::vector<std::pair<Int, Int>>& bounds);

  std::vector<Int> makeValVector(const std::vector<VarViewId>& inputVars) {
    std::vector<Int> vals(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      EXPECT_NE(inputVars.at(i), NULL_ID);
      vals.at(i) = _solver->lowerBound(inputVars.at(i));
    }
    return vals;
  }

  [[nodiscard]] static VarViewId getMinVarViewId(
      const std::vector<VarViewId>& vars);

  [[nodiscard]] static VarViewId getMaxVarViewId(
      const std::vector<VarViewId>& vars);

  void expectNextInput(const std::vector<VarViewId>& inputVars,
                       Invariant& invariant) const;

  static Int increaseNextVal(const std::vector<std::pair<Int, Int>>& bounds,
                             std::vector<Int>& vals);

  Int increaseNextVal(const std::vector<VarViewId>& varIds,
                      std::vector<Int>& inputVals) const;

  void setVarVals(Timestamp ts, const std::vector<VarViewId>& inputVars,
                  const std::vector<Int>& vals);

  void setVarVals(const std::vector<VarViewId>& inputVars,
                  const std::vector<Int>& vals);

  void notifyInputsChanged(Timestamp ts, Invariant& invariant,
                           const std::vector<VarViewId>& inputVars) const;

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

  static std::pair<Int, Int> genBounds(Int lb, Int ub);

  static std::pair<Int, Int> genBounds() {
    return genBounds(std::numeric_limits<Int>::min(),
                     std::numeric_limits<Int>::max());
  }

  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    _solver = std::make_unique<Solver>();
    generateState = GenerateState::RANDOM;
    binaryDist = std::uniform_int_distribution<unsigned char>(0, 1);
  }
};


}  // namespace atlantis::testing
