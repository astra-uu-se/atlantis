#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "utils/idMap.hpp"

namespace {
class IdMapTest : public ::testing::Test {};

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
  invToVar[id2] = data2;
  EXPECT_EQ(invToVar[id2], data2);
  invToVar.register_idx(id3);
  invToVar.register_idx(id4);
  invToVar[id1] = data1;
  invToVar[id2] = data1;  // intentional
  invToVar[id3] = data3;
  invToVar[id4] = data4;

  EXPECT_EQ(invToVar[id1], data1);
  EXPECT_EQ(invToVar[id2], data1);  // intentional
  EXPECT_EQ(invToVar[id3], data3);
  EXPECT_EQ(invToVar[id4], data4);
}

TEST_F(IdMapTest, ID2Vectortest) {
  IdMap<VarId, std::vector<VarId>> varToVector(10000);

  VarId id1(1);
  VarId id2(2);
  VarId id3(3);
  VarId id4(4);
  // values to store
  varToVector.register_idx(id1);
  varToVector.register_idx(id2);
  varToVector[id2].emplace_back(21);
  varToVector[id2].emplace_back(22);
  EXPECT_EQ(varToVector[id2].size(), size_t(2));
  EXPECT_EQ(varToVector[id1].size(), size_t(0));
}

}  // namespace
