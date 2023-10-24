#pragma once

#include <algorithm>

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"

namespace atlantis::search::neighbourhoods {

class CircuitNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVar> _vars;

 public:
  CircuitNeighbourhood(std::vector<search::SearchVar>&&);

  ~CircuitNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVar>& coveredVars() const override { return _vars; }

 private:
  [[nodiscard]] Int getNode(size_t nodeIdx) const noexcept;
  [[nodiscard]] size_t getNodeIdx(Int node) const noexcept;
};

}  // namespace atlantis::search::neighbourhoods
