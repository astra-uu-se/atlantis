#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"

namespace search::neighbourhoods {

class RandomNeighbourhood : public Neighbourhood {
 public:
  RandomNeighbourhood(std::vector<VarId> variables, const Engine& engine)
      : _variables(std::move(variables)), _engine(engine) {}

  ~RandomNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

 private:
  std::vector<VarId> _variables;
  const Engine& _engine;

  Int randomValue(RandomProvider& random, VarId variable);
};

}  // namespace search::neighbourhoods
