#pragma once

#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search::neighbourhoods {

class CircuitNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVar> _vars;
  Int _offset;

 public:
  explicit CircuitNeighbourhood(std::vector<search::SearchVar>&&, Int offset);

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

 private:
  [[nodiscard]] Int idx2Node(size_t nodeIdx) noexcept;
  [[nodiscard]] size_t node2Idx(Int node) noexcept;
};

}  // namespace atlantis::search::neighbourhoods
