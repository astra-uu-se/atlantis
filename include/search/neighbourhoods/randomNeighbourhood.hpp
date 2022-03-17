#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/variableStore.hpp"

namespace search::neighbourhoods {

class RandomNeighbourhood : public Neighbourhood {
 public:
  RandomNeighbourhood(std::vector<VarId> variables, RandomProvider& random,
                      VariableStore& variableStore)
      : _variables(std::move(variables)),
        _random(random),
        _variableStore(variableStore) {}

  ~RandomNeighbourhood() override = default;

  void initialise(AssignmentModification& modifications) override;
  std::unique_ptr<search::Move> randomMove() override;

 private:
  std::vector<VarId> _variables;
  RandomProvider& _random;
  VariableStore& _variableStore;

  Int randomValue(VarId variable);
};

}  // namespace search::neighbourhoods
