#pragma once

#include "neighbourhood.hpp"
namespace search::neighbourhoods {

/**
 * This is the default neighbourhood used for variables which are not defined
 * by an implicit constraint. Moves by this neighbourhood are selecting a random
 * variable and assigning a new random value to it.
 */
class MaxViolatingNeighbourhood : public Neighbourhood {
 private:
  const std::vector<VarId>& _variables;

 public:
  explicit MaxViolatingNeighbourhood(const std::vector<VarId>& variables)
      : _variables(variables) {}

  void initialize(PropagationEngine& engine) override;
  Move randomMove(PropagationEngine& engine) override;
};

}  // namespace search::neighbourhoods