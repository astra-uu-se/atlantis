#include <gtest/gtest.h>

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
  const AnnealingScheduleFactory factory;
  const auto schedule = factory.create(0);
  auto* loop = dynamic_cast<ScheduleLoop*>(schedule.get());
  EXPECT_NE(loop, nullptr);
  auto& loopInner = loop->inner();
  auto const* sequence = dynamic_cast<ScheduleSequence const*>(&loopInner);
  EXPECT_NE(sequence, nullptr);
  EXPECT_EQ(sequence->size(), 2);
  auto& heatingRef = sequence->at(0);
  auto const* heating = dynamic_cast<GeometricHeatingSchedule const*>(&heatingRef);
  EXPECT_NE(heating, nullptr);
  EXPECT_EQ(heating->heatingRate(), 1.2);
  EXPECT_EQ(heating->minimumUphillAcceptanceRatio(), 0.75);

  auto& coolingRef = sequence->at(1);
  auto const* cooling = dynamic_cast<GeometricCoolingSchedule const*>(&coolingRef);
  EXPECT_NE(cooling, nullptr);
  EXPECT_EQ(cooling->coolingRate(), 0.99);
  EXPECT_EQ(cooling->successiveFutileRoundsThreshold(), 4);
}

}  // namespace atlantis::testing
