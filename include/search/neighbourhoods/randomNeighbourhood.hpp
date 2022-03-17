#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"

namespace search::neighbourhoods {

class RandomNeighbourhood : public Neighbourhood {
 public:
  RandomNeighbourhood(std::vector<VarId> variables, RandomProvider& random,
                      const PropagationEngine& engine)
      : _variables(std::move(variables)),
        _random(random),
        _engine(engine) {}

  ~RandomNeighbourhood() override = default;

  void initialise(AssignmentModification& modifications) override;
  std::unique_ptr<search::Move> randomMove() override;

 private:
  std::vector<VarId> _variables;
  RandomProvider& _random;
  const PropagationEngine& _engine;

  Int randomValue(VarId variable);
};

}  // namespace search::neighbourhoods
