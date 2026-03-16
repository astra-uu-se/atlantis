#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class DummyAnnealingSchedule : public AnnealingSchedule {
public:
    std::vector<double> temperatures{};
    std::vector<std::shared_ptr<RoundStatistics>> roundStatistics{};

    MOCK_METHOD(double, temperature, (), (const override));
    MOCK_METHOD(bool, frozen, (), (const override));

    void start(double initialTemperature) override { temperatures.emplace_back(initialTemperature); }
    void nextRound(const std::shared_ptr<RoundStatistics>& statistics) override { roundStatistics.emplace_back(statistics); }
    std::unique_ptr<AnnealingSchedule> clone() const override { return std::make_unique<DummyAnnealingSchedule>(); }
};

}  // namespace atlantis::testing