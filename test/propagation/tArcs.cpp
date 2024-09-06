#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/propagation/arcs.hpp"
#include "atlantis/types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

TEST(ArcsTest, IncomingArcContainerTest) {
  invariant::IncomingArcContainer container;
  EXPECT_EQ(container.numArcs(), size_t(0));
  const size_t numDynamic = 100;
  const size_t numStatic = 100;

  for (size_t i = 0; i < numDynamic; i++) {
    EXPECT_EQ(container.emplaceDynamic(VarId{i % 10}, i % 5), LocalId{i});
    EXPECT_EQ(container.numArcs(), i + 1);
  }

  for (size_t i = numDynamic; i < numDynamic + numStatic; i++) {
    EXPECT_EQ(container.emplaceStatic(VarId{i % 10}), LocalId{i});
    EXPECT_EQ(container.numArcs(), i + 1);
    EXPECT_THROW(container.emplaceDynamic(VarId{i + 1}, i % 5),
                 OutOfOrderIndexRegistration);
  }
  size_t i = 0;
  for (const auto& varId : container.incomingStatic()) {
    EXPECT_EQ(varId, VarId{i % 10});
    ++i;
  }
  i = 0;
  for (const auto& [varId, outgoingDynamicArcIndex] :
       container.incomingDynamic()) {
    EXPECT_EQ(varId, VarId{i % 10});
    EXPECT_EQ(outgoingDynamicArcIndex, i % 5);
    ++i;
  }
}

TEST(ArcsTest, OutgoingDynamicArcContainer) {
  var::OutgoingArc arc(LocalId{1}, InvariantId{2});
  EXPECT_EQ(arc.localId(), LocalId{1});
  EXPECT_EQ(arc.invariantId(), InvariantId{2});
  arc.setInvariantNullId();
  EXPECT_EQ(arc.invariantId(), NULL_ID);
}

class OutgoingDynamicArcContainerTest : public ::testing::Test {};

TEST_F(OutgoingDynamicArcContainerTest, emplaceBack) {
  var::OutgoingDynamicArcContainer container;
  const size_t numArcs = 100;
  const Timestamp ts = 1;
  EXPECT_TRUE(container.empty());
  for (size_t i = 0; i < numArcs; i++) {
    container.emplaceBack(LocalId{i}, InvariantId{1 + (i % 10)});
    EXPECT_FALSE(container.empty());
    EXPECT_EQ(container.size(), i + 1);
    EXPECT_EQ(container.numActive(ts), 0);
    EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
    EXPECT_EQ(container[i].localId(), LocalId{i});
    EXPECT_EQ(container[i].invariantId(), InvariantId{1 + (i % 10)});
    EXPECT_EQ(container.at(i).localId(), LocalId{i});
    EXPECT_EQ(container.at(i).invariantId(), InvariantId{1 + (i % 10)});
  }
}

TEST_F(OutgoingDynamicArcContainerTest, makeActive) {
  var::OutgoingDynamicArcContainer container;
  const size_t numArcs = 10;
  for (size_t i = 0; i < numArcs; i++) {
    container.emplaceBack(LocalId{i}, InvariantId{1 + (i % 10)});
  }
  std::vector<size_t> indices(numArcs);
  std::iota(indices.begin(), indices.end(), 0);
  for (Timestamp ts = 1; ts < 3; ++ts) {
    std::random_shuffle(indices.begin(), indices.end());
    for (size_t i = 0; i < numArcs; ++i) {
      for (size_t iter = 0; iter < 3; ++iter) {
        for (size_t j = i + 1; j < numArcs; ++j) {
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

TEST_F(OutgoingDynamicArcContainerTest, makeInactive) {
  var::OutgoingDynamicArcContainer container;
  const size_t numArcs = 10;
  for (size_t i = 0; i < numArcs; i++) {
    container.emplaceBack(LocalId{i}, InvariantId{1 + (i % 10)});
  }
  std::vector<size_t> indices(numArcs);
  std::iota(indices.begin(), indices.end(), 0);
  for (Timestamp ts = 1; ts < 3; ++ts) {
    for (size_t i = 0; i < numArcs; i++) {
      container.makeActive(ts, i);
    }
    for (size_t i = 0; i < numArcs; ++i) {
      for (size_t iter = 0; iter < 3; ++iter) {
        for (size_t j = i + 1; j < numArcs; ++j) {
          EXPECT_TRUE(container.isActive(ts, indices[j]));
        }
        if (iter == 0) {
          EXPECT_EQ(container.numActive(ts), numArcs - i);
        } else {
          EXPECT_EQ(container.numActive(ts), numArcs - i - 1);
        }
        EXPECT_EQ(container.isActive(ts, indices[i]), iter == 0);
        container.makeInactive(ts, indices[i]);
        EXPECT_EQ(container.numActive(ts), numArcs - i - 1);
        for (size_t j = 0; j <= i; ++j) {
          EXPECT_FALSE(container.isActive(ts, indices[j]));
        }
        EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
      }
      EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
    }
  }
}

TEST_F(OutgoingDynamicArcContainerTest, makeAllInactive) {
  var::OutgoingDynamicArcContainer container;
  const size_t numArcs = 10;
  for (size_t i = 0; i < numArcs; i++) {
    container.emplaceBack(LocalId{i}, InvariantId{1 + (i % 10)});
  }
  std::vector<size_t> indices(numArcs);
  std::iota(indices.begin(), indices.end(), 0);
  for (Timestamp ts = 1; ts < 3; ++ts) {
    for (size_t i : indices) {
      for (size_t j = 0; j < i; j++) {
        container.makeActive(ts, j);
      }
      container.makeAllInactive(ts);
      EXPECT_EQ(container.numActive(ts), 0);
      for (size_t j = 0; j < numArcs; j++) {
        EXPECT_FALSE(container.isActive(ts, j));
      }
    }
  }
}

TEST_F(OutgoingDynamicArcContainerTest, setNullId) {
  var::OutgoingDynamicArcContainer container;
  const size_t numArcs = 10;
  for (size_t i = 0; i < numArcs; i++) {
    container.emplaceBack(LocalId{i}, InvariantId{1 + (i % 10)});
  }
  std::vector<size_t> indices(numArcs);
  std::iota(indices.begin(), indices.end(), 0);
  for (size_t i = 0; i < numArcs; i++) {
    for (size_t j = i; j < numArcs; j++) {
      EXPECT_EQ(container[indices[j]].invariantId(),
                InvariantId{1 + (indices[j] % 10)});
    }
    container.setNullId(i);
    for (size_t j = 0; j <= i; j++) {
      EXPECT_EQ(container[indices[j]].invariantId(), NULL_ID);
    }
  }
}

TEST_F(OutgoingDynamicArcContainerTest, commitIf) {
  var::OutgoingDynamicArcContainer container;
  const size_t numArcs = 10;
  std::vector<size_t> indices(numArcs);
  std::iota(indices.begin(), indices.end(), 0);
  for (size_t i = 0; i < numArcs; i++) {
    container.emplaceBack(LocalId{i}, InvariantId{1 + (i % 10)});
  }
  EXPECT_EQ(container.numActive(NULL_TIMESTAMP), 0);
  std::vector<bool> isActive(numArcs, false);

  Timestamp ts = 1;
  for (; ts <= 10; ++ts) {
    std::random_shuffle(indices.begin(), indices.end());
    for (size_t i = 0; i < std::min<size_t>(3, numArcs); i++) {
      container.makeActive(ts, indices[i]);
      isActive[indices[i]] = true;
      EXPECT_TRUE(container.isActive(ts, indices[i]));
    }
    container.commitIf(ts);
    for (size_t i = 0; i < numArcs; i++) {
      EXPECT_EQ(container.isActive(ts, i), isActive[i]) << "i: " << i << "; "
                                                        << "ts: " << ts;
    }
  }

  container.makeAllInactive(ts);
  container.commitIf(ts);
  ts++;

  for (size_t i = 0; i < numArcs; i++) {
    EXPECT_FALSE(container.isActive(ts + 1, i));
  }

  for (size_t i = 0; i < numArcs; i++) {
    container.makeActive(ts, i);
    isActive[i] = true;
  }

  container.commitIf(ts);
  ts++;

  for (size_t i = 0; i < numArcs; i++) {
    EXPECT_TRUE(container.isActive(ts + 1, i));
  }

  const Timestamp end = ts + 10;

  for (; ts < end; ++ts) {
    std::random_shuffle(indices.begin(), indices.end());
    for (size_t i = 0; i < std::min<size_t>(3, numArcs); i++) {
      container.makeInactive(ts, indices[i]);
      isActive[indices[i]] = false;
      EXPECT_FALSE(container.isActive(ts, indices[i]));
    }
    container.commitIf(ts);
    for (size_t i = 0; i < numArcs; i++) {
      EXPECT_EQ(container.isActive(ts, i), isActive[i]) << "i: " << i << "; "
                                                        << "ts: " << ts;
    }
  }
}

}  // namespace atlantis::testing
