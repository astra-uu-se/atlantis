#pragma once

#include "search/move.hpp"

namespace search::neighbourhoods {

class Neighbourhood {
 public:
  virtual ~Neighbourhood() = default;

  /**
   * Initialise an assignment.
   *
   * @param modifications The modifications to the assignment.
   */
  virtual void initialise(AssignmentModification& modifications) = 0;

  /**
   * @return A probed random move. Ownership should be with the neighbourhood,
   * as it can reuse the same allocated move. Hence, the lifetime of the
   * returned value is tied to the lifetime of the neighbourhood.
   */
  virtual search::Move* randomMove() = 0;
};

}  // namespace search::neighbourhoods