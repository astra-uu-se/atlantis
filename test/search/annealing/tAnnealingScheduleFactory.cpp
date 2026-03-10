#include <gtest/gtest.h>

#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/annealing/geometricCoolingSchedule.hpp"
#include "atlantis/search/annealing/geometricHeatingSchedule.hpp"
#include "atlantis/search/annealing/scheduleLoop.hpp"
#include "atlantis/search/annealing/scheduleSequence.hpp"
#include "atlantis/search/annealing/types.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class TestAnnealingScheduleFactory : public ::testing::Test {
};

TEST_F(TestAnnealingScheduleFactory, makeDefaultAnnealingSchedule) {
  const AnnealingScheduleFactory factory({});
  auto schedule = factory.create(0);
  auto* loop = dynamic_cast<ScheduleLoop*>(schedule.get());
  EXPECT_NE(loop, nullptr);
  auto& loopInner = loop->inner();
  auto* sequence = dynamic_cast<ScheduleSequence*>(&loopInner);
  EXPECT_NE(sequence, nullptr);
  EXPECT_EQ(sequence->size(), 2);
  auto& heatingRef = sequence->at(0);
  auto* heating = dynamic_cast<GeometricHeatingSchedule*>(&heatingRef);
  EXPECT_NE(heating, nullptr);
  EXPECT_EQ(heating->temperature(), 1.2);
  auto& coolingRef = sequence->at(1);
  auto* cooling = dynamic_cast<GeometricCoolingSchedule*>(&coolingRef);
  EXPECT_NE(cooling, nullptr);
  EXPECT_EQ(cooling->temperature(), 0.99);
}

}  // namespace atlantis::testing
