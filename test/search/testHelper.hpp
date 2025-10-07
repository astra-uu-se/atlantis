#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::testing {

using namespace atlantis::search;
using namespace atlantis::search::neighborhoods;

class AlwaysAcceptingAnnealer : public Annealer {
 public:
  AlwaysAcceptingAnnealer(RandomProvider& random, std::unique_ptr<AnnealingSchedule>&& schedule,
                          const Assignment& assignment)
      : Annealer(random, std::move(schedule), assignment) {}

 protected:
  [[nodiscard]] bool accept(Int) override { return true; }
};

class MockNeighborhood : public Neighborhood {
 public:
  MOCK_METHOD(void, initialize, (RandomProvider&, Assignment&), (override));

  MOCK_METHOD(size_t, randomMove, (RandomProvider&, Assignment&), (override));

  MOCK_METHOD(void, commitIf, (const Assignment&), (override));

  MOCK_METHOD(const std::vector<SearchVar>&, coveredVars, (), (const override));
};

}  // namespace atlantis::testing