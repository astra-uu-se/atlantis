#pragma once

#include <memory>
#include <vector>

#include "neighbourhood.hpp"

namespace search::neighbourhoods {

class NeighbourhoodCombinator : public Neighbourhood {
 private:
  std::vector<std::unique_ptr<Neighbourhood>> _neighbourhoods;

 public:
  explicit NeighbourhoodCombinator(
      std::vector<std::unique_ptr<Neighbourhood>> neighbourhoods)
      : _neighbourhoods(std::move(neighbourhoods)) {}

  void initialize(PropagationEngine& engine) override;
  Move randomMove(PropagationEngine& engine) override;
};

}  // namespace search::neighbourhoods