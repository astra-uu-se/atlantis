#pragma once

#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search::neighbourhoods {

class AllDifferentUniformNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVar> _vars;
  std::vector<Int> _domain;
  size_t _moveVarIdx;
  size_t _moveValIdx;
  Timestamp _curTimestamp;
  bool _hasFreeValues;

 private:
  size_t swapValues(RandomProvider&, IAssignment& assignment);

  size_t assignValue(RandomProvider&, IAssignment& assignment);

 public:
  AllDifferentUniformNeighbourhood(std::vector<search::SearchVar>&& vars,
                                   std::vector<Int>&& domain);

  void initialise(RandomProvider&, IAssignment&) override;

  size_t randomMove(RandomProvider&, IAssignment&) override;

  void commitIf(const IAssignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighbourhoods
