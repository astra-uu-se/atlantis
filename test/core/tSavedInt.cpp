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
    virtual void SetUp() {
        std::random_device rd;
        gen = std::mt19937(rd());
    }
    std::mt19937 gen;
};

TEST_F(SavedIntTest, SavedInt) {
    std::uniform_int_distribution<> distribution(INT_MIN, INT_MAX - 1);
    
    Timestamp timestamp = std::min(0, distribution(gen));
    Int value = distribution(gen);

    SavedInt savedInt = SavedInt(timestamp, value);
    EXPECT_EQ(savedInt.getValue(timestamp), value);
    EXPECT_EQ(savedInt.peekValue(timestamp), value);
    EXPECT_EQ(savedInt.getValue(timestamp + 1), value);
    EXPECT_EQ(savedInt.peekValue(timestamp + 1), value);
}

TEST_F(SavedIntTest, SavedIntSetValue) {
    std::uniform_int_distribution<> distribution1(INT_MIN, 10000);
    std::uniform_int_distribution<> distribution2(10001, INT_MAX);
    
    Timestamp timestamp1 = std::min(0, distribution1(gen));
    Int value1 = distribution1(gen);
    
    Timestamp timestamp2 = distribution2(gen);
    Int value2 = distribution2(gen);

    SavedInt savedInt = SavedInt(timestamp1, value1);
    EXPECT_EQ(savedInt.getValue(timestamp1), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value1);
    EXPECT_EQ(savedInt.getValue(timestamp2), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value1);

    savedInt.setValue(timestamp2, value2);

    EXPECT_EQ(savedInt.getValue(timestamp1), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value1);
    EXPECT_EQ(savedInt.getValue(timestamp2), value2);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value2);

}

TEST_F(SavedIntTest, SavedIntValue) {
    std::uniform_int_distribution<> distribution(INT_MIN + 10, INT_MAX - 10);
    
    Timestamp timestamp = std::min(0, distribution(gen));
    Int value = distribution(gen);
    
    SavedInt savedInt = SavedInt(timestamp, value);
    EXPECT_EQ(savedInt.getValue(timestamp), value);
    EXPECT_EQ(savedInt.peekValue(timestamp), value);
    EXPECT_EQ(savedInt.getValue(timestamp), value);
    EXPECT_EQ(savedInt.peekValue(timestamp), value);

    Timestamp ts;
    Int v;
    for (int i = -10; i <= 10; ++i) {
        if (i == 0) {
            continue;
        }
        v = value + i;
        ts = timestamp + i;
        savedInt.setValue(timestamp, value);

        EXPECT_EQ(savedInt.getValue(timestamp), value);
        EXPECT_EQ(savedInt.peekValue(timestamp), value);
        EXPECT_EQ(savedInt.getValue(ts), v);
        EXPECT_EQ(savedInt.peekValue(ts), v);
    }
}

TEST_F(SavedIntTest, SavedIntCommitValue) {
    std::uniform_int_distribution<> distribution1(INT_MIN, 10000);
    std::uniform_int_distribution<> distribution2(10001, INT_MAX);
    
    Timestamp timestamp1 = std::min(0, distribution1(gen));
    Int value1 = distribution1(gen);
    
    Timestamp timestamp2 = distribution2(gen);
    Int value2 = distribution2(gen);

    SavedInt savedInt = SavedInt(timestamp1, value1);
    
    EXPECT_EQ(savedInt.getValue(timestamp1), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value1);
    EXPECT_EQ(savedInt.getValue(timestamp2), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value1);

    savedInt.commitValue(value2);

    EXPECT_EQ(savedInt.getValue(timestamp1), value2);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value2);
    EXPECT_EQ(savedInt.getValue(timestamp2), value2);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value2);
}

TEST_F(SavedIntTest, SavedIntCommit) {
    std::uniform_int_distribution<> distribution1(INT_MIN, 10000);
    std::uniform_int_distribution<> distribution2(10001, INT_MAX);
    
    Timestamp timestamp1 = std::min(0, distribution1(gen));
    Int value1 = distribution1(gen);
    
    Timestamp timestamp2 = distribution2(gen);
    Int value2 = distribution2(gen);

    SavedInt savedInt = SavedInt(timestamp1, value1);
    
    EXPECT_EQ(savedInt.getValue(timestamp1), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value1);
    EXPECT_EQ(savedInt.getValue(timestamp2), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value1);

    savedInt.setValue(timestamp2,value2);

    savedInt.commitIf(timestamp2);

    EXPECT_EQ(savedInt.getValue(timestamp1), value2);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value2);
    EXPECT_EQ(savedInt.getValue(timestamp2), value2);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value2);
}

TEST_F(SavedIntTest, SavedIntCommitIf) {
    std::uniform_int_distribution<> distribution1(INT_MIN, 10000);
    std::uniform_int_distribution<> distribution2(10001, INT_MAX);
    
    Timestamp timestamp1 = std::min(0, distribution1(gen));
    Int value1 = distribution1(gen);
    
    Timestamp timestamp2 = distribution2(gen);
    Int value2 = distribution2(gen);

    SavedInt savedInt = SavedInt(timestamp1, value1);

    savedInt.commitIf(timestamp2);

    EXPECT_EQ(savedInt.getValue(timestamp1), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value1);
    EXPECT_EQ(savedInt.getValue(timestamp2), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value1);

    savedInt.commitIf(timestamp1);

    EXPECT_EQ(savedInt.getValue(timestamp1), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value1);
    EXPECT_EQ(savedInt.getValue(timestamp2), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value1);

    savedInt.setValue(timestamp2,value2);

    savedInt.commitIf(timestamp2);

    EXPECT_EQ(savedInt.getValue(timestamp1), value1);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value1);
    EXPECT_EQ(savedInt.getValue(timestamp2), value2);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value2);

    savedInt.commitIf(timestamp1);

    EXPECT_EQ(savedInt.getValue(timestamp1), value2);
    EXPECT_EQ(savedInt.peekValue(timestamp1), value2);
    EXPECT_EQ(savedInt.getValue(timestamp2), value2);
    EXPECT_EQ(savedInt.peekValue(timestamp2), value2);
}

} //namespace