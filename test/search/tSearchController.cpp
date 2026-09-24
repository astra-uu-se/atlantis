#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/searchController.hpp"
#include "atlantis/search/threadController.hpp"
#include "testHelper.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

TEST(SearchControllerTest, StopsWhenThreadControllerRequestsStop) {
  propagation::Solver solver;
  const auto neighborhood = std::make_shared<MockNeighborhood>();
  static const std::vector<SearchVar> coveredVars{};
  EXPECT_CALL(*neighborhood, coveredVars())
      .WillRepeatedly(::testing::ReturnRef(coveredVars));

  const Assignment assignment(
      solver, neighborhood, propagation::VAR_VIEW_NULL_ID,
      propagation::VAR_VIEW_NULL_ID, ObjectiveDirection::NONE, 0);
  const auto controllerState = std::make_shared<ThreadController>(1);
  std::shared_ptr<const bool> shouldStop{nullptr};
  SearchController controller(false, std::optional<std::chrono::milliseconds>{},
                              shouldStop, controllerState);

  EXPECT_TRUE(controller.shouldRun(assignment));

  controllerState->requestStop();

  EXPECT_FALSE(controller.shouldRun(assignment));
}

}  // namespace atlantis::testing
