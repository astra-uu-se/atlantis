#pragma once

#include "move.hpp"

namespace search {

class Annealer {
 public:
  /**
   * Determine whether @p move should be committed to the assignment.
   *
   * @tparam N The size of the move.
   * @param move The move itself.
   * @return True if @p move should be committed, false otherwise.
   */
  template <unsigned int N>
  bool acceptMove(Move<N>) {
    return false;
  }
};

}