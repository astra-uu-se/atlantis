#pragma once

#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search::neighbourhoods {

class IntLinEqNeighbourhood : public Neighbourhood {
 private:
  std::vector<Int> _coeffs;
  std::vector<search::SearchVar> _vars;
  Int _bound;
  const propagation::SolverBase& _solver;
  std::vector<size_t> _indices;

 public:
  IntLinEqNeighbourhood(std::vector<Int>&& coeffs,
                        std::vector<search::SearchVar>&& vars, Int bound,
                        const propagation::SolverBase& solver);

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighbourhoods
