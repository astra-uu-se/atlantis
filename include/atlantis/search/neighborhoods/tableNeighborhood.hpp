#pragma once

#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {

class TableNeighborhood : public Neighborhood {
  std::vector<SearchVar> _vars;
  std::vector<std::vector<Int>> _table;
  propagation::CommittableInt _index;

 public:
  explicit TableNeighborhood(std::vector<SearchVar>&& vars,
                             std::vector<std::vector<Int>>&& table);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  void commitIf(const Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighborhoods
