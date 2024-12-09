#pragma once

#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search::neighbourhoods {

class IntLinEqNeighbourhood : public Neighbourhood {
 private:
  std::vector<Int> _coeffs;
  std::vector<search::SearchVar> _vars;
  Int _offset;
  std::vector<size_t> _indices;

 public:
  IntLinEqNeighbourhood(std::vector<Int>&& coeffs,
                        std::vector<search::SearchVar>&& vars, Int offset);

  void initialize(RandomProvider&, IAssignment&) override;

  size_t randomMove(RandomProvider&, IAssignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighbourhoods
