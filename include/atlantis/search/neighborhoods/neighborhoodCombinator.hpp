#pragma once

#include <memory>
#include <random>
#include <vector>

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/types.hpp"

namespace atlantis::logging {
class Logger;
}

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {

class NeighborhoodCombinator : public Neighborhood {
  std::vector<std::shared_ptr<Neighborhood>> _neighborhoods;
  std::vector<SearchVar> _vars;
  std::discrete_distribution<size_t> _neighborhoodDistribution;
  Timestamp _curTimestamp;
  size_t _curNeighborhood;

 public:
  explicit NeighborhoodCombinator(
      std::vector<std::shared_ptr<Neighborhood>>&& neighborhoods);

  void initialize(RandomProvider&, Assignment&) override;
  size_t randomMove(RandomProvider&, Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

  void printNeighborhood(logging::Logger&) const;

  void commitIf(const Assignment&) override;
};

}  // namespace atlantis::search::neighborhoods
