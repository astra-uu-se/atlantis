#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"

namespace atlantis::search::neighbourhoods {

class RandomNeighbourhood : public Neighbourhood {
 private:
  std::vector<SearchVar> _vars;
  const propagation::SolverBase& _solver;

 public:
  RandomNeighbourhood(std::vector<SearchVar> vars,
                      const propagation::SolverBase& solver)
      : _vars(std::move(vars)), _solver(solver) {}

  ~RandomNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVar>& coveredVars() const override { return _vars; }
};

}  // namespace atlantis::search::neighbourhoods
