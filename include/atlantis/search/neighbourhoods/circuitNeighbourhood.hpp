#pragma once

#include <algorithm>

#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "neighbourhood.hpp"

namespace atlantis::search::neighbourhoods {

class CircuitNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVar> _vars;

 public:
  explicit CircuitNeighbourhood(std::vector<search::SearchVar>&&);

  ~CircuitNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

 private:
  [[nodiscard]] static Int getNode(size_t nodeIdx) noexcept;
  [[nodiscard]] static size_t getNodeIdx(Int node) noexcept;
};

}  // namespace atlantis::search::neighbourhoods
