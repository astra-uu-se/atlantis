#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"

namespace search::neighbourhoods {

class RandomNeighbourhood : public Neighbourhood {
 private:
  std::vector<SearchVariable> _variables;
  const Engine& _engine;

 public:
  RandomNeighbourhood(std::vector<SearchVariable> variables,
                      const Engine& engine)
      : _variables(std::move(variables)), _engine(engine) {}

  ~RandomNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVariable> & coveredVariables() const override {
    return _variables;
  }
};

}  // namespace search::neighbourhoods
