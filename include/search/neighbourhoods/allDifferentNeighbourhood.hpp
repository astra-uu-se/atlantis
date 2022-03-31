#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"

namespace search::neighbourhoods {

class AllDifferentNeighbourhood : public Neighbourhood {
 private:
  std::vector<VarId> _variables;
  std::vector<Int> _domain;
  const Engine& _engine;

  std::vector<size_t> _domIndices{};
  Int _offset{0};
  size_t _freeVariables{0};

 public:
  AllDifferentNeighbourhood(std::vector<VarId> variables,
                            std::vector<Int> domain, const Engine& engine);

  ~AllDifferentNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

 private:
  bool swapValues(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer);
  bool assignValue(RandomProvider& random, Assignment& assignment,
                   Annealer& annealer);
};

}  // namespace search::neighbourhoods
