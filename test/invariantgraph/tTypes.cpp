#include <gtest/gtest.h>

#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

TEST(InvariantNodeIdTest, bitMask) {
  const size_t leftMostTrue = size_t{1} << (sizeof(size_t) * CHAR_BIT - 2);
  size_t id{1};
  while ((id & leftMostTrue) == size_t{0}) {
    const InvariantNodeId invId(id, false);
    EXPECT_TRUE(invId.isInvariant());
    EXPECT_EQ(size_t(invId), id);
    EXPECT_NE(invId, NULL_NODE_ID);

    const InvariantNodeId implId(id, true);
    EXPECT_TRUE(implId.isImplicitConstraint());
    EXPECT_EQ(size_t(implId), id);
    EXPECT_NE(implId, NULL_NODE_ID);

    EXPECT_NE(invId, implId);
    EXPECT_GT(id, size_t(0));
    size_t newId = (id << 1) | size_t{1};
    EXPECT_GT(newId, id);
    id = newId;
  }
  ASSERT_EQ(id, (~size_t{0}) >> 1);
}
}  // namespace atlantis::testing
