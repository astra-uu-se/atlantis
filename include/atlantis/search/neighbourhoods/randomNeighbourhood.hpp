#pragma once

#include <vector>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighbourhoods//neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search::neighbourhoods {

class RandomNeighbourhood : public Neighbourhood {
 private:
  std::vector<SearchVar> _vars;

 public:
  RandomNeighbourhood(std::vector<SearchVar>&& vars);

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;

  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighbourhoods
