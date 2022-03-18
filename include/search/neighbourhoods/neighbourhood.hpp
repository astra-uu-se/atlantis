#pragma once

#include "search/annealer.hpp"
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
   * Make a random move on @p assignment. After a move is constructed, the
   * decision whether to apply it is taken by @p annealer.
   */
  virtual void randomMove(Assignment& assignment, Annealer* annealer) = 0;

 protected:
  template <unsigned int N>
  void maybeCommit(Move<N> move, Assignment& assignment, Annealer* annealer) {
    if (annealer->acceptMove(move)) {
      move.commit(assignment);
    }
  }
};

}  // namespace search::neighbourhoods