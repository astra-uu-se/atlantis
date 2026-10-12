#include <gtest/gtest.h>

#include <numeric>
#include <random>
#include <vector>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/propagation/edges.hpp"
#include "atlantis/types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

TEST(EdgeTest, IncomingEdgeContainerTest) {
  invariant::IncomingEdgeContainer container;
  EXPECT_EQ(container.numEdges(), size_t(0));
  const size_t numDynamic = 100;
  const size_t numStatic = 100;

  for (size_t i = 0; i < numDynamic; i++) {
    EXPECT_EQ(container.emplaceDynamic(VarId{i % 10}, i % 5), i);
    EXPECT_EQ(container.numEdges(), i + 1);
  }

  for (size_t i = numDynamic; i < numDynamic + numStatic; i++) {
    EXPECT_EQ(container.emplaceStatic(VarId{i % 10}), i);
    EXPECT_EQ(container.numEdges(), i + 1);
    EXPECT_THROW(container.emplaceDynamic(VarId{i + 1}, i % 5),
                 OutOfOrderIndexRegistration);
  }
  size_t i = 0;
  for (const auto& varId : container.staticEdges()) {
    EXPECT_EQ(varId, VarId{i % 10});
    ++i;
  }
  i = 0;
  for (const auto& [varId, outgoingDynamicEdgeIndex] :
       container.dynamicEdges()) {
    EXPECT_EQ(varId, VarId{i % 10});
    EXPECT_EQ(outgoingDynamicEdgeIndex, i % 5);
    ++i;
  }
}

TEST(EdgeTest, OutgoingDynamicEdgeContainer) {
  var::OutgoingEdge edge(LocalId{1}, InvariantId{2});
  EXPECT_EQ(edge.localId(), LocalId{1});
  EXPECT_EQ(edge.invariantId(), InvariantId{2});
  edge.setInvariantNullId();
  EXPECT_EQ(edge.invariantId(), NULL_ID);
}

class OutgoingDynamicEdgeContainerTest : public ::testing::Test {};

TEST_F(OutgoingDynamicEdgeContainerTest, emplaceBack) {
  var::OutgoingDynamicEdgeContainer container;
  const size_t numEdges = 100;
  const Timestamp ts = 1;
  EXPECT_TRUE(container.empty());
  for (size_t i = 0; i < numEdges; i++) {
    container.emplaceBack(i, InvariantId{1 + (i % 10)});
    EXPECT_FALSE(container.empty());
    EXPECT_EQ(container.size(), i + 1);
    EXPECT_EQ(container.numActive(ts), 0);
    EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
    EXPECT_EQ(container[i].localId(), i);
    EXPECT_EQ(container[i].invariantId(), InvariantId{1 + (i % 10)});
    EXPECT_EQ(container.at(i).localId(), i);
    EXPECT_EQ(container.at(i).invariantId(), InvariantId{1 + (i % 10)});
  }
}

TEST_F(OutgoingDynamicEdgeContainerTest, makeActive) {
  var::OutgoingDynamicEdgeContainer container;
  const size_t numEdges = 10;
  for (size_t i = 0; i < numEdges; i++) {
    container.emplaceBack(i, InvariantId{1 + (i % 10)});
  }
  std::vector<size_t> indices(numEdges);
  std::iota(indices.begin(), indices.end(), 0);
  for (Timestamp ts = 1; ts < 3; ++ts) {
    std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));
    for (size_t i = 0; i < numEdges; ++i) {
      for (size_t iter = 0; iter < 3; ++iter) {
        for (size_t j = i + 1; j < numEdges; ++j) {
          EXPECT_FALSE(container.isActive(ts, indices[j]));
        }
        if (iter == 0) {
          EXPECT_EQ(container.numActive(ts), i);
        } else {
          EXPECT_EQ(container.numActive(ts), i + 1);
        }
        EXPECT_EQ(container.isActive(ts, indices[i]), iter > 0);
        container.makeActive(ts, indices[i]);
        EXPECT_EQ(container.numActive(ts), i + 1);
        for (size_t j = 0; j <= i; ++j) {
          EXPECT_TRUE(container.isActive(ts, indices[j]));
        }
        EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
      }
    }
  }
}

TEST_F(OutgoingDynamicEdgeContainerTest, makeInactive) {
  var::OutgoingDynamicEdgeContainer container;
  const size_t numEdges = 10;
  for (size_t i = 0; i < numEdges; i++) {
    container.emplaceBack(i, InvariantId{1 + (i % 10)});
  }
  std::vector<size_t> indices(numEdges);
  std::iota(indices.begin(), indices.end(), 0);
  for (Timestamp ts = 1; ts < 3; ++ts) {
    for (size_t i = 0; i < numEdges; i++) {
      container.makeActive(ts, i);
    }
    for (size_t i = 0; i < numEdges; ++i) {
      for (size_t iter = 0; iter < 3; ++iter) {
        for (size_t j = i + 1; j < numEdges; ++j) {
          EXPECT_TRUE(container.isActive(ts, indices[j]));
        }
        if (iter == 0) {
          EXPECT_EQ(container.numActive(ts), numEdges - i);
        } else {
          EXPECT_EQ(container.numActive(ts), numEdges - i - 1);
        }
        EXPECT_EQ(container.isActive(ts, indices[i]), iter == 0);
        container.makeInactive(ts, indices[i]);
        EXPECT_EQ(container.numActive(ts), numEdges - i - 1);
        for (size_t j = 0; j <= i; ++j) {
          EXPECT_FALSE(container.isActive(ts, indices[j]));
        }
        EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
      }
      EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
    }
  }
}

TEST_F(OutgoingDynamicEdgeContainerTest, makeAllInactive) {
  var::OutgoingDynamicEdgeContainer container;
  const size_t numEdges = 10;
  for (size_t i = 0; i < numEdges; i++) {
    container.emplaceBack(i, InvariantId{1 + (i % 10)});
  }
  std::vector<size_t> indices(numEdges);
  std::iota(indices.begin(), indices.end(), 0);
  for (Timestamp ts = 1; ts < 3; ++ts) {
    for (size_t i : indices) {
      for (size_t j = 0; j < i; j++) {
        container.makeActive(ts, j);
      }
      container.makeAllInactive(ts);
      EXPECT_EQ(container.numActive(ts), 0);
      for (size_t j = 0; j < numEdges; j++) {
        EXPECT_FALSE(container.isActive(ts, j));
      }
    }
  }
}

TEST_F(OutgoingDynamicEdgeContainerTest, setNullId) {
  var::OutgoingDynamicEdgeContainer container;
  const size_t numEdges = 10;
  for (size_t i = 0; i < numEdges; i++) {
    container.emplaceBack(i, InvariantId{1 + (i % 10)});
  }
  std::vector<size_t> indices(numEdges);
  std::iota(indices.begin(), indices.end(), 0);
  for (size_t i = 0; i < numEdges; i++) {
    for (size_t j = i; j < numEdges; j++) {
      EXPECT_EQ(container[indices[j]].invariantId(),
                InvariantId{1 + (indices[j] % 10)});
    }
    container.setNullId(i);
    for (size_t j = 0; j <= i; j++) {
      EXPECT_EQ(container[indices[j]].invariantId(), NULL_ID);
    }
  }
}

TEST_F(OutgoingDynamicEdgeContainerTest, commitIf) {
  var::OutgoingDynamicEdgeContainer container;
  const size_t numEdges = 10;
  std::vector<size_t> indices(numEdges);
  std::iota(indices.begin(), indices.end(), 0);
  for (size_t i = 0; i < numEdges; i++) {
    container.emplaceBack(i, InvariantId{1 + (i % 10)});
  }
  EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
  std::vector<bool> isActive(numEdges, false);

  Timestamp ts = 1;
  for (; ts <= 10; ++ts) {
    std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));
    for (size_t i = 0; i < std::min<size_t>(3, numEdges); i++) {
      container.makeActive(ts, indices[i]);
      isActive[indices[i]] = true;
      EXPECT_TRUE(container.isActive(ts, indices[i]));
    }
    container.commitIf(ts);
    for (size_t i = 0; i < numEdges; i++) {
      EXPECT_EQ(container.isActive(ts, i), isActive[i]) << "i: " << i << "; "
                                                        << "ts: " << ts;
    }
  }

  container.makeAllInactive(ts);
  container.commitIf(ts);
  ts++;

  for (size_t i = 0; i < numEdges; i++) {
    EXPECT_FALSE(container.isActive(ts + 1, i));
  }

  for (size_t i = 0; i < numEdges; i++) {
    container.makeActive(ts, i);
    isActive[i] = true;
  }

  container.commitIf(ts);
  ts++;

  for (size_t i = 0; i < numEdges; i++) {
    EXPECT_TRUE(container.isActive(ts + 1, i));
  }

  const Timestamp end = ts + 10;

  for (; ts < end; ++ts) {
    std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));
    for (size_t i = 0; i < std::min<size_t>(3, numEdges); i++) {
      container.makeInactive(ts, indices[i]);
      isActive[indices[i]] = false;
      EXPECT_FALSE(container.isActive(ts, indices[i]));
    }
    container.commitIf(ts);
    for (size_t i = 0; i < numEdges; i++) {
      EXPECT_EQ(container.isActive(ts, i), isActive[i]) << "i: " << i << "; "
                                                        << "ts: " << ts;
    }
  }
}

}  // namespace atlantis::testing
