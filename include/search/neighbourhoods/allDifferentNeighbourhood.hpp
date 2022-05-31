#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"
#include "utils/flowNetwork.hpp"

namespace search::neighbourhoods {

class AllDifferentNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVariable> _variables;
  utils::FlowNetwork _flowNetwork{2, 0, 1};

  std::vector<Int> _values;

  Int _minVal;
  Int _maxVal;
  std::unordered_set<Int> _freeValues;

 public:
  explicit AllDifferentNeighbourhood(std::vector<search::SearchVariable> variables);

  ~AllDifferentNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVariable> & coveredVariables() const override {
    return _variables;
  }

 private:
  bool swapValues(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer);
  bool assignValue(RandomProvider& random, Assignment& assignment,
                   Annealer& annealer);
  size_t createMatching(AssignmentModifier& modifications);
};

}  // namespace search::neighbourhoods
