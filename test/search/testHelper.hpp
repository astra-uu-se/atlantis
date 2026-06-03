#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/invariantgraph/gecodeSolver.hpp"
#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/metaheuristic.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::testing {

using namespace atlantis::search;
using namespace atlantis::search::neighborhoods;

class AlwaysAcceptingMetaHeuristic : public MetaHeuristic {
 public:
  AlwaysAcceptingMetaHeuristic() : MetaHeuristic() {}

  void start() override {}
  [[nodiscard]] bool acceptMove(const Cost&) override { return true; }
  [[nodiscard]] bool isFinished() const override { return false; }
};

class MockNeighborhood : public Neighborhood {
 public:
  MOCK_METHOD(void, initialize, (RandomProvider&, Assignment&), (override));

  MOCK_METHOD(size_t, randomMove, (RandomProvider&, Assignment&), (override));

  MOCK_METHOD(void, commitIf, (const Assignment&), (override));

  MOCK_METHOD(const std::vector<SearchVar>&, coveredVars, (), (const override));
};

}  // namespace atlantis::testing