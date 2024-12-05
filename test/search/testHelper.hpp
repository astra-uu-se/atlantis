#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/search/annealer.hpp"
#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::testing {

using namespace atlantis::search;
using namespace atlantis::search::neighbourhoods;

class AlwaysAcceptingAnnealer : public search::Annealer {
 public:
  AlwaysAcceptingAnnealer(RandomProvider& random, AnnealingSchedule& schedule,
                          const IAssignment& assignment)
      : Annealer(random, schedule, assignment) {}

 protected:
  [[nodiscard]] bool accept(Int) override { return true; }
};

class MockNeighbourhood : public Neighbourhood {
 public:
  MOCK_METHOD(void, initialise, (RandomProvider&, IAssignment&), (override));

  MOCK_METHOD(size_t, randomMove, (RandomProvider&, IAssignment&), (override));

  MOCK_METHOD(const std::vector<SearchVar>&, coveredVars, (), (const override));
};

}  // namespace atlantis::testing