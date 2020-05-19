#include <iostream>
#include <random>
#include <vector>
#include "gtest/gtest.h"
#include "domain/intDomain.hpp"
#include <bits/stdc++.h> 
#include <iostream>

// The fixture for testing class IntDomain. From google test primer.
class IntDomainTest : public ::testing::Test {
protected:
    // You can remove any or all of the following functions if its body
    // is empty.

    IntDomainTest() {
        std::random_device rd;
        gen = std::mt19937(rd());
        // You can do set-up work for each test here.
    }

    virtual ~IntDomainTest() {
        // You can do clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:
    virtual void SetUp() {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    virtual void TearDown() {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    std::mt19937 gen;
    // Objects declared here can be used by all tests in the test case for IntDomain.
};

// Test case must be called the class above
// Also note: use TEST_F instead of TEST to access the test fixture (from google test primer)
TEST_F(IntDomainTest, EmptyIntDomain) {
    auto vector = std::vector<std::pair<int,int>>();
    IntDomain intDomain = IntDomain(vector);
    EXPECT_EQ(0, intDomain.domainSize);
    
    std::uniform_int_distribution<> distribution(INT_MIN, INT_MAX);
    for (size_t i = 0; i < 10; i++) {
        int value = i == 0 ? value - 1 : (i == 1 ? value + 1 : distribution(gen));
        EXPECT_FALSE(intDomain.containsValue(value));
        auto containingBound = intDomain.findContainingBound(value);
        EXPECT_TRUE(std::get_if<IntDomain::OutOfBounds>(&containingBound));
    }
}

TEST_F(IntDomainTest, SingleIntDomain) {
    std::uniform_int_distribution<> distribution(INT_MIN + 1, INT_MAX - 1);
    
    int value = distribution(gen);
    auto vector = std::vector<std::pair<int,int>> { intBound(value, value) };
    IntDomain intDomain = IntDomain(vector);
    EXPECT_EQ(1, intDomain.domainSize);
    
    EXPECT_TRUE(intDomain.containsValue(value));
    
    auto containingBound = intDomain.findContainingBound(value);
    EXPECT_TRUE(std::get_if<IntDomain::FoundBound>(&containingBound));
    IntDomain::FoundBound foundBound = std::get<IntDomain::FoundBound>(containingBound);
    EXPECT_EQ(0, foundBound.index);

    EXPECT_FALSE(intDomain.containsValue(value - 1));
    auto oneLessOutOfBoundsSmall = intDomain.findContainingBound(value - 1);
    EXPECT_TRUE(std::get_if<IntDomain::OutOfBoundsSmall>(&oneLessOutOfBoundsSmall));

    std::uniform_int_distribution<> distributionLesser(INT_MIN, value - 1);
    for (size_t i = 0; i < 10; i++) {
        int lesserValue = distributionLesser(gen);
        EXPECT_FALSE(intDomain.containsValue(lesserValue));
        auto containingOutOfBoundsSmall = intDomain.findContainingBound(lesserValue);
        EXPECT_TRUE(std::get_if<IntDomain::OutOfBoundsSmall>(&containingOutOfBoundsSmall));
    }

    EXPECT_FALSE(intDomain.containsValue(value + 1));
    auto oneMoreOutOfBoundsLarge = intDomain.findContainingBound(value + 1);
    EXPECT_TRUE(std::get_if<IntDomain::OutOfBoundsLarge>(&oneMoreOutOfBoundsLarge));

    std::uniform_int_distribution<> distributionGreater(value + 1, INT_MAX);
    for (size_t i = 0; i < 10; i++) {
        int greaterValue = i == 0 ? value + 1 : distributionGreater(gen);
        EXPECT_FALSE(intDomain.containsValue(greaterValue));
        auto containingOutOfBoundsLarge = intDomain.findContainingBound(greaterValue);
        EXPECT_TRUE(std::get_if<IntDomain::OutOfBoundsLarge>(&containingOutOfBoundsLarge));
    }

}

TEST_F(IntDomainTest, SingleRangeIntDomain) {
    int rangeSize = 10;
    std::uniform_int_distribution<> distribution(INT_MIN + 1, INT_MAX - 1 - rangeSize);
    
    int minValue = distribution(gen);
    int maxValue = minValue + rangeSize - 1;

    auto vector = std::vector<std::pair<int,int>> { intBound(minValue, maxValue) };
    IntDomain intDomain = IntDomain(vector);
    
    EXPECT_EQ(rangeSize, intDomain.domainSize);

    for (int value = minValue; value <= maxValue; value++) {    
        EXPECT_TRUE(intDomain.containsValue(value));
        auto containingBound = intDomain.findContainingBound(value);
        EXPECT_TRUE(std::get_if<IntDomain::FoundBound>(&containingBound));
        IntDomain::FoundBound foundBound = std::get<IntDomain::FoundBound>(containingBound);
        EXPECT_EQ(0, foundBound.index);
    }

    std::uniform_int_distribution<> distributionLesser(INT_MIN, minValue - 1);
    for (size_t i = 0; i < 10; i++) {
        int lesserValue = distributionLesser(gen);
        EXPECT_FALSE(intDomain.containsValue(lesserValue));
        auto containingOutOfBounds = intDomain.findContainingBound(lesserValue);
        EXPECT_TRUE(std::get_if<IntDomain::OutOfBoundsSmall>(&containingOutOfBounds));
    }

    std::uniform_int_distribution<> distributionGreater(maxValue + 1, INT_MAX);
    for (size_t i = 0; i < 10; i++) {
        int greaterValue = distributionGreater(gen);
        EXPECT_FALSE(intDomain.containsValue(greaterValue));
        auto containingOutOfBounds = intDomain.findContainingBound(greaterValue);
        EXPECT_TRUE(std::get_if<IntDomain::OutOfBoundsLarge>(&containingOutOfBounds));
    }

}

TEST_F(IntDomainTest, IntDomain) {
    int rangeSize = 10;
    int rangeCount = 10;
    std::vector<std::pair<int,int>> vector;
    vector.reserve(rangeSize);
    for (int i = 0; i < rangeCount; i++) {
        vector.emplace_back(intBound(i * (rangeSize + 1), i * (rangeSize + 1) + rangeSize - 1));
    }

    IntDomain intDomain = IntDomain(vector);
    
    EXPECT_EQ(rangeSize * rangeCount, intDomain.domainSize);

    for (int i = 0; i < rangeCount; i++) {
        for (int value = i * (rangeSize + 1); value <= i * (rangeSize + 1) + rangeSize - 1; value++) {
            EXPECT_TRUE(intDomain.containsValue(value));
            auto containingBound = intDomain.findContainingBound(value);
            EXPECT_TRUE(std::get_if<IntDomain::FoundBound>(&containingBound));
            IntDomain::FoundBound foundBound = std::get<IntDomain::FoundBound>(containingBound);
            EXPECT_EQ(i, foundBound.index);
        }
    }

	for (int i = 0; i < rangeCount - 1; i++) {
		int betweenValue = i * (rangeSize + 1) + rangeSize;
		EXPECT_FALSE(intDomain.containsValue(betweenValue));
		auto containingBound = intDomain.findContainingBound(betweenValue);
		EXPECT_TRUE(std::get_if<IntDomain::BetweenBounds>(&containingBound));
		IntDomain::BetweenBounds betweenBounds = std::get<IntDomain::BetweenBounds>(containingBound);
		EXPECT_EQ(i, betweenBounds.lower);
		EXPECT_EQ(i+1, betweenBounds.upper);
	}

    std::uniform_int_distribution<> distributionLesser(INT_MIN, -1);
    for (size_t i = 0; i < 10; i++) {
        int lesserValue = distributionLesser(gen);
        EXPECT_FALSE(intDomain.containsValue(lesserValue));
        auto containingOutOfBounds = intDomain.findContainingBound(lesserValue);
        EXPECT_TRUE(std::get_if<IntDomain::OutOfBoundsSmall>(&containingOutOfBounds));
    }

    std::uniform_int_distribution<> distributionGreater(rangeCount * (rangeSize + 2) + rangeSize - 1, INT_MAX);
    for (size_t i = 0; i < 10; i++) {
        int greaterValue = distributionGreater(gen);
        EXPECT_FALSE(intDomain.containsValue(greaterValue));
        auto containingOutOfBounds = intDomain.findContainingBound(greaterValue);
        EXPECT_TRUE(std::get_if<IntDomain::OutOfBoundsLarge>(&containingOutOfBounds));
    }

}


// }  // namespace - could surround IntDomainTest in a namespace