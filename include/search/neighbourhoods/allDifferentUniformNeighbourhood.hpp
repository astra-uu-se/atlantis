#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"

namespace atlantis::search::neighbourhoods {

class AllDifferentUniformNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVariable> _variables;
  std::vector<Int> _domain;
  const propagation::Engine& _engine;

  std::vector<size_t> _domIndices{};
  Int _offset{0};
  size_t _freeVariables{0};

 public:
  AllDifferentUniformNeighbourhood(
      std::vector<search::SearchVariable>&& variables, std::vector<Int> domain,
      const propagation::Engine& engine);

  ~AllDifferentUniformNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVariable>& coveredVariables() const override {
    return _variables;
  }

 private:
  bool swapValues(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer);
  bool assignValue(RandomProvider& random, Assignment& assignment,
                   Annealer& annealer);
};

}  // namespace atlantis::search::neighbourhoods
