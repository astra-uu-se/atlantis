#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"

namespace atlantis::search::neighbourhoods {

class RandomNeighbourhood : public Neighbourhood {
 private:
  std::vector<SearchVariable> _variables;
  const propagation::SolverBase& _solver;

 public:
  RandomNeighbourhood(std::vector<SearchVariable> variables,
                      const propagation::SolverBase& solver)
      : _variables(std::move(variables)), _solver(solver) {}

  ~RandomNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVariable> & coveredVariables() const override {
    return _variables;
  }
};

}  // namespace atlantis::search::neighbourhoods
