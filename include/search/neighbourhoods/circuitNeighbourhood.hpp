#pragma once

#include <numeric>

#include "invariantgraph/parseHelper.hpp"
#include "neighbourhood.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"

namespace search::neighbourhoods {

class CircuitNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVariable> _variables;
  const Engine& _engine;

  std::vector<size_t> _domIndices{};
  std::vector<std::vector<Int>> _domains;
  size_t _freeVariables{0};

 public:
  CircuitNeighbourhood(std::vector<search::SearchVariable>&& variables,
                       std::vector<std::vector<Int>>&& domains,
                       const Engine& engine);

  ~CircuitNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVariable>& coveredVariables() const override {
    return _variables;
  }

 private:
  bool threeOpt(RandomProvider& random, Assignment& assignment,
                Annealer& annealer);
};

}  // namespace search::neighbourhoods
