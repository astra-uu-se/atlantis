#pragma once

#include <memory>
#include <vector>

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::search::neighborhoods {

class NeighborhoodCombinator : public Neighborhood {
 private:
  std::vector<std::shared_ptr<Neighborhood>> _neighborhoods;
  std::vector<SearchVar> _vars;
  std::discrete_distribution<size_t> _neighborhoodDistribution;
  Timestamp _curTimestamp;
  size_t _curNeighborhood;

 public:
  explicit NeighborhoodCombinator(
      std::vector<std::shared_ptr<Neighborhood>>&& neighborhoods);

  void initialize(RandomProvider&, IAssignment&) override;
  size_t randomMove(RandomProvider&, IAssignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

  void printNeighborhood(logging::Logger&);

  void commitIf(const IAssignment&) override;
};

}  // namespace atlantis::search::neighborhoods
