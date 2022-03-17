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
   * @return A probed random move.
   */
  virtual std::unique_ptr<search::Move> randomMove() = 0;
};

}  // namespace search::neighbourhoods