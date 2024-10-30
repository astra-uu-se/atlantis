#pragma once

#include "atlantis/search/annealer.hpp"

namespace atlantis::testing {

class AlwaysAcceptingAnnealer : public search::Annealer {
 public:
  AlwaysAcceptingAnnealer(const search::Assignment& assignment,
                          search::RandomProvider& random,
                          search::AnnealingSchedule& schedule)
      : Annealer(assignment, random, schedule) {}

 protected:
  [[nodiscard]] bool accept(Int) override { return true; }
};

}  // namespace atlantis::testing