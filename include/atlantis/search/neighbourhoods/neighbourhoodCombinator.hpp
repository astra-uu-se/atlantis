#pragma once

#include <ostream>
#include <random>
#include <typeinfo>

#include "logging/logger.hpp"
#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "utils/type.hpp"

namespace atlantis::search::neighbourhoods {

class NeighbourhoodCombinator : public Neighbourhood {
 private:
  std::vector<std::shared_ptr<Neighbourhood>> _neighbourhoods;
  std::vector<SearchVar> _vars;
  std::discrete_distribution<size_t> _neighbourhoodDistribution;

 public:
  explicit NeighbourhoodCombinator(
      std::vector<std::shared_ptr<Neighbourhood>>&& neighbourhoods);

  ~NeighbourhoodCombinator() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

  void printNeighbourhood(logging::Logger&);

 private:
  Neighbourhood& selectNeighbourhood(RandomProvider& random);
};

}  // namespace atlantis::search::neighbourhoods
