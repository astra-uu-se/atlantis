#pragma once

#include <ostream>
#include <random>

#include "logging/logger.hpp"
#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"

namespace search::neighbourhoods {

class NeighbourhoodCombinator : public Neighbourhood {
 private:
  std::vector<std::shared_ptr<Neighbourhood>> _neighbourhoods;
  std::vector<SearchVariable> _variables;
  std::discrete_distribution<size_t> _neighbourhoodDistribution;

 public:
  explicit NeighbourhoodCombinator(
      std::vector<std::shared_ptr<Neighbourhood>>&& neighbourhoods);

  ~NeighbourhoodCombinator() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVariable>& coveredVariables() const override {
    return _variables;
  }

  void printNeighbourhood(logging::Logger&);

 private:
  Neighbourhood& selectNeighbourhood(RandomProvider& random);
};

}  // namespace search::neighbourhoods
