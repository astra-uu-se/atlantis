#pragma once

#include "core/propagationEngine.hpp"
#include "search/move.hpp"

namespace search::neighbourhoods {

class Neighbourhood {
 public:
  virtual ~Neighbourhood() = default;

  virtual void initialise(PropagationEngine& engine) = 0;
  virtual Move randomMove(PropagationEngine& engine) = 0;
};

}  // namespace search::neighbourhoods