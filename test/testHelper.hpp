#pragma once
#ifndef CBLS_TEST
#define CBLS_TEST
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

class InvariantTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  std::mt19937 gen;
  std::default_random_engine rng;
  std::vector<std::pair<PropagationMode, OutputToInputMarkingMode>>
      propMarkModes{
          {PropagationMode::INPUT_TO_OUTPUT, OutputToInputMarkingMode::NONE}/*,
          {PropagationMode::OUTPUT_TO_INPUT, OutputToInputMarkingMode::NONE},
          {PropagationMode::OUTPUT_TO_INPUT,
           OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC},
          {PropagationMode::OUTPUT_TO_INPUT,
           OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION}*/};

  template <class T>
  void testNotifications(T* invariant, PropagationMode propMode,
                         OutputToInputMarkingMode markingMode,
                         const size_t numNextInputCalls,
                         const VarId modifiedVarId, const Int modifiedVal,
                         const VarId queryVarId) {
    EXPECT_CALL(*invariant, recompute(testing::_, testing::_))
        .Times(AtLeast(1));
    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    if (!engine->isOpen()) {
      engine->open();
    }
    engine->setPropagationMode(propMode);
    engine->setOutputToInputMarkingMode(markingMode);
    engine->close();

    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, nextInput(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(*invariant, nextInput(testing::_, testing::_))
          .Times(numNextInputCalls);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    }

    engine->beginMove();
    engine->setValue(modifiedVarId, modifiedVal);
    engine->endMove();

    engine->beginProbe();
    engine->query(queryVarId);
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