#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

// below function returns the subsets of vector origin.
template <class T>
std::vector<std::vector<T>> subsets(std::vector<T>& origin) {
  std::vector<T> subset;
  std::vector<std::vector<T>> res;
  std::deque<std::pair<size_t, size_t>> q;
  res.emplace_back(std::vector<T>{});
  q.emplace_back(std::pair<size_t, size_t>{size_t(0), 0 + 1});
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
  PropagationMode propMode;
  OutputToInputMarkingMode markingMode;
  size_t numNextInputCalls;
  VarId modifiedVarId;
  Int modifiedVal;
  VarId queryVarId;
};

class InvariantTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
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

  template <class T>
  void testNotifications(T* invariant, NotificationData data) {
    EXPECT_CALL(*invariant, recompute(testing::_)).Times(AtLeast(1));
    EXPECT_CALL(*invariant, commit(testing::_)).Times(AtLeast(1));

    if (!engine->isOpen()) {
      engine->open();
    }
    engine->setPropagationMode(data.propMode);
    engine->setOutputToInputMarkingMode(data.markingMode);
    engine->close();

    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, nextInput(testing::_)).Times(0);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant, notifyInputChanged(testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(*invariant, nextInput(testing::_))
          .Times(data.numNextInputCalls);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_)).Times(1);

      EXPECT_CALL(*invariant, notifyInputChanged(testing::_, testing::_))
          .Times(AtMost(1));
    }

    engine->beginMove();
    engine->setValue(data.modifiedVarId, data.modifiedVal);
    engine->endMove();

    engine->beginProbe();
    engine->query(data.queryVarId);
    engine->endProbe();
    engine->open();
  }

 public:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
  }
};