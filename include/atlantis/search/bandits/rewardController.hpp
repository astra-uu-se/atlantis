#pragma once
#include "pullResults.hpp"

namespace atlantis::search {

class RewardController {
public:

  virtual ~RewardController() = default;

  [[nodiscard]] virtual double getReward(size_t arm, const PullResults& results) = 0;
};

}  // namespace atlantis::search
