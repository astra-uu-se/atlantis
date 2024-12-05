#pragma once

#include <memory>
#include <vector>

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::search::neighbourhoods {

class NeighbourhoodCombinator : public Neighbourhood {
 private:
  std::vector<std::shared_ptr<Neighbourhood>> _neighbourhoods;
  std::vector<SearchVar> _vars;
  std::discrete_distribution<size_t> _neighbourhoodDistribution;
  Timestamp _curTimestamp;
  size_t _curNeighbourhood;

 public:
  explicit NeighbourhoodCombinator(
      std::vector<std::shared_ptr<Neighbourhood>>&& neighbourhoods);

  void initialise(RandomProvider&, IAssignment&) override;
  size_t randomMove(RandomProvider&, IAssignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

  void printNeighbourhood(logging::Logger&);

  void commitIf(const IAssignment&) override;
};

}  // namespace atlantis::search::neighbourhoods
