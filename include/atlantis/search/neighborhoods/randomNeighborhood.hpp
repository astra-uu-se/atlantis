#pragma once

#include <vector>

#include "atlantis/search/neighborhoods/neighborhood.hpp"

namespace atlantis::search {
class SearchVar;
class Assignment;
class RandomProvider;
}  // namespace atlantis::search

namespace atlantis::search::neighborhoods {

class RandomNeighborhood : public Neighborhood {
  std::vector<SearchVar> _vars;

 public:
  explicit RandomNeighborhood(std::vector<SearchVar>&& vars);

  void initialize(RandomProvider&, Assignment& assignment) override;

  size_t randomMove(RandomProvider&, Assignment& assignment) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighborhoods
