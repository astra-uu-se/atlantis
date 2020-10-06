#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

#include "core/idMap.hpp"
#include "gtest/gtest.h"

namespace {
class IdMapTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
  }
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(IdMapTest, ID2IDtest) {
  IdMap<InvariantId, VarId> invToVar(10000);

  InvariantId id1(1);
  InvariantId id2(2);
  InvariantId id3(3);
  InvariantId id4(4);
  // values to store
  VarId data1(11);
  VarId data2(12);
  VarId data3(13);
  VarId data4(14);
  invToVar.register_idx(id1);
  invToVar.register_idx(id2);
  invToVar.at(id2) = data2;
  EXPECT_EQ(invToVar.at(id2), data2);
  invToVar.register_idx(id3);
  invToVar.register_idx(id4);
  invToVar.at(id1) = data1;
  invToVar.at(id2) = data1;  // intentional
  invToVar.at(id3) = data3;
  invToVar.at(id4) = data4;

  EXPECT_EQ(invToVar.at(id1), data1);
  EXPECT_EQ(invToVar.at(id2), data1);  // intentional
  EXPECT_EQ(invToVar.at(id3), data3);
  EXPECT_EQ(invToVar.at(id4), data4);
}

}  // namespace
