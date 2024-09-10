#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <random>
#include <vector>

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
  propagation::VarViewId modifiedVarId{NULL_ID};
  Int modifiedVal{0};
  propagation::VarViewId queryVarId{NULL_ID};
};

class InvariantTest : public ::testing::Test {
 protected:
  std::shared_ptr<propagation::Solver> _solver;
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

  std::vector<Int> createInputVals(const std::vector<VarViewId>& inputVars) {
    std::vector<Int> inputVals(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      inputVals.at(i) = _solver->lowerBound(inputVars.at(i));
    }
    return inputVals;
  }

  bool increaseNextVal(const std::vector<propagation::VarViewId>& varIds,
                       std::vector<Int>& inputVals) const {
    EXPECT_EQ(varIds.size(), inputVals.size());
    for (Int i = static_cast<Int>(inputVals.size() - 1); i >= 0; --i) {
      if (varIds.at(i) == propagation::NULL_ID) {
        continue;
      }
      if (inputVals.at(i) < _solver->upperBound(varIds.at(i))) {
        ++inputVals.at(i);
        return true;
      }
      inputVals.at(i) = _solver->lowerBound(varIds.at(i));
    }
    return false;
  }

  void setVarVals(const std::vector<propagation::VarViewId>& varIds,
                  const std::vector<Int>& vals) const {
    EXPECT_EQ(varIds.size(), vals.size());
    for (size_t i = 0; i < varIds.size(); ++i) {
      if (varIds.at(i) != propagation::NULL_ID) {
        _solver->setValue(varIds.at(i), vals.at(i));
      }
    }
  }

  size_t trySetMinDiffInputVarVal(const std::vector<VarViewId>& inputVars,
                                  std::vector<Int>& inputVals) {
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

 public:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    _solver = std::make_unique<propagation::Solver>();
  }
};

}  // namespace atlantis::testing
