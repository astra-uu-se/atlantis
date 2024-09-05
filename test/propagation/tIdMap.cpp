#include <gtest/gtest.h>

#include <vector>

#include "atlantis/propagation/utils/idMap.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IdMapTest : public ::testing::Test {};

/**
 *  Testing constructor
 */

TEST_F(IdMapTest, ID2IDtest) {
  IdMap<VarId> invToVar(10000);

  InvariantId id1(0);
  InvariantId id2(1);
  InvariantId id3(2);
  InvariantId id4(3);
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
  IdMap<std::vector<VarId>> varToVector(10000);

  VarId id1(0);
  VarId id2(1);
  // values to store
  varToVector.register_idx(id1);
  varToVector.register_idx(id2);
  varToVector[id2].emplace_back(21);
  varToVector[id2].emplace_back(22);
  EXPECT_EQ(varToVector[id2].size(), size_t(2));
  EXPECT_EQ(varToVector[id1].size(), size_t(0));
}

}  // namespace atlantis::testing
