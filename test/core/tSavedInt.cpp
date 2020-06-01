#include <iostream>
#include <random>
#include <vector>
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"

// The fixture for testing class IntDomain. From google test primer.
namespace {
class SavedIntTest : public ::testing::Test {
protected:
    int INT_MIN = std::numeric_limits<int>::min();
    int INT_MAX = std::numeric_limits<int>::max();

    virtual void SetUp() {
        std::random_device rd;
        gen = std::mt19937(rd());
    }
    std::mt19937 gen;
};

TEST_F(SavedIntTest, SavedIntConstructor) {
    std::uniform_int_distribution<> distribution(INT_MIN, INT_MAX - 1);
    
    Timestamp initTime = std::min(0, distribution(gen));
    Int value = distribution(gen);

    SavedInt savedInt = SavedInt(initTime, value);
    EXPECT_EQ(savedInt.getValue(initTime), value);
    EXPECT_EQ(savedInt.peekValue(initTime), value);
    EXPECT_EQ(savedInt.getValue(initTime + 1), value);
    EXPECT_EQ(savedInt.peekValue(initTime + 1), value);
}

TEST_F(SavedIntTest, SavedIntSetValue) {
    std::uniform_int_distribution<> distribution1(INT_MIN, 10000);
    std::uniform_int_distribution<> distribution2(10001, INT_MAX);
    
    Timestamp initTime = std::min(0, distribution1(gen));
    Int initValue = distribution1(gen);
    
    Timestamp nextTime = distribution2(gen);
    Int nextValue = distribution2(gen);

    SavedInt savedInt = SavedInt(initTime, initValue);
    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), initValue);

    EXPECT_EQ(savedInt.getValue(nextTime), initValue);
    EXPECT_EQ(savedInt.getValue(initTime), initValue);
    
    savedInt.setValue(nextTime, nextValue);

    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), nextValue);
    
    EXPECT_EQ(savedInt.getValue(nextTime), nextValue);

    EXPECT_EQ(savedInt.getValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), initValue);
    
}
TEST_F(SavedIntTest, SavedIntValue) {
    std::uniform_int_distribution<> distribution(INT_MIN + 10, INT_MAX - 10);
    
    Timestamp initTime = std::min(0, distribution(gen));
    Int initValue = distribution(gen);
    
    SavedInt savedInt = SavedInt(initTime, initValue);
    EXPECT_EQ(savedInt.getValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    
    Timestamp nextTime;
    Int nextValue;
    for (int i = -10; i <= 10; ++i) {
        if (i == 0) {
            continue;
        }
        nextTime = initTime + i;
        nextValue = initValue + i;
        savedInt.setValue(nextTime, nextValue);

        EXPECT_EQ(savedInt.peekValue(initTime), initValue);
        EXPECT_EQ(savedInt.peekValue(nextTime), nextValue);
        
        EXPECT_EQ(savedInt.getValue(nextTime), nextValue);

        EXPECT_EQ(savedInt.getValue(initTime), initValue);
        EXPECT_EQ(savedInt.peekValue(initTime), initValue);
        EXPECT_EQ(savedInt.peekValue(nextTime), initValue);
    }
}

TEST_F(SavedIntTest, SavedIntCommitValue) {
    std::uniform_int_distribution<> distribution1(INT_MIN, 10000);
    std::uniform_int_distribution<> distribution2(10001, INT_MAX);
    
    Timestamp initTime = std::min(0, distribution1(gen));
    Int initValue = distribution1(gen);
    
    Timestamp nextTime = distribution2(gen);
    Int commitedValue = distribution2(gen);

    SavedInt savedInt = SavedInt(initTime, initValue);
    
    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), initValue);
    
    savedInt.commitValue(commitedValue);

    EXPECT_EQ(savedInt.peekValue(initTime), commitedValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), initValue);
}
/*
TEST_F(SavedIntTest, SavedIntCommit) {
    std::uniform_int_distribution<> distribution1(INT_MIN, 10000);
    std::uniform_int_distribution<> distribution2(10001, INT_MAX);
    
    Timestamp initTime = std::min(0, distribution1(gen));
    Int initValue = distribution1(gen);
    
    Timestamp nextTime = distribution2(gen);
    Int commitedValue = distribution2(gen);

    SavedInt savedInt = SavedInt(initTime, initValue);
    
    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), initValue);
    
    savedInt.setValue(nextTime, commitedValue);

    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), commitedValue);

    savedInt.commitIf(nextTime);

    //EXPECT_EQ(savedInt.peekValue(initTime), commitedValue);
    //EXPECT_EQ(savedInt.peekValue(nextTime), commitedValue);
}
/*

TEST_F(SavedIntTest, SavedIntCommitIf) {
    std::uniform_int_distribution<> distribution1(INT_MIN, 10000);
    std::uniform_int_distribution<> distribution2(10001, INT_MAX);
    
    Timestamp initTime = std::min(0, distribution1(gen));
    Int initValue = distribution1(gen);
    
    Timestamp nextTime = distribution2(gen);
    Int value2 = distribution2(gen);

    SavedInt savedInt = SavedInt(initTime, initValue);

    savedInt.commitIf(nextTime);

    EXPECT_EQ(savedInt.getValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.getValue(nextTime), initValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), initValue);

    savedInt.commitIf(initTime);

    EXPECT_EQ(savedInt.getValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.getValue(nextTime), initValue);
    EXPECT_EQ(savedInt.peekValue(nextTime), initValue);

    savedInt.setValue(nextTime,value2);

    savedInt.commitIf(nextTime);

    EXPECT_EQ(savedInt.getValue(initTime), initValue);
    EXPECT_EQ(savedInt.peekValue(initTime), initValue);
    EXPECT_EQ(savedInt.getValue(nextTime), value2);
    EXPECT_EQ(savedInt.peekValue(nextTime), value2);

    savedInt.commitIf(initTime);

    EXPECT_EQ(savedInt.getValue(initTime), value2);
    EXPECT_EQ(savedInt.peekValue(initTime), value2);
    EXPECT_EQ(savedInt.getValue(nextTime), value2);
    EXPECT_EQ(savedInt.peekValue(nextTime), value2);
}
*/
} //namespace