#pragma once

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"

namespace search::neighbourhoods {

class CircuitNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVariable> _variables;

 public:
  CircuitNeighbourhood(std::vector<search::SearchVariable>&& variables);

  ~CircuitNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVariable>& coveredVariables() const override {
    return _variables;
  }

 private:
  [[nodiscard]] Int getNode(size_t nodeIdx) const noexcept;
  [[nodiscard]] size_t getNodeIdx(Int node) const noexcept;
};

}  // namespace search::neighbourhoods
