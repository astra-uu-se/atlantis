#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"

namespace search::neighbourhoods {

class NeighbourhoodCombinator : public Neighbourhood {
 private:
  std::vector<std::unique_ptr<Neighbourhood>> _neighbourhoods;

 public:
  explicit NeighbourhoodCombinator(
      std::vector<std::unique_ptr<Neighbourhood>> neighbourhoods)
      : _neighbourhoods(std::move(neighbourhoods)) {
    assert(!_neighbourhoods.empty());
  }

  ~NeighbourhoodCombinator() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;
};

}  // namespace search::neighbourhoods
