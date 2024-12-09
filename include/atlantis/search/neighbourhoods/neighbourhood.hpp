#pragma once

#include <vector>

#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search::neighbourhoods {

class Neighbourhood {
 public:
  virtual ~Neighbourhood() = default;

  /**
   * Initialize an assignment.
   *
   * @param random The source of randomness.
   */
  virtual void initialize(RandomProvider& random, IAssignment& assignment) = 0;

  /**
   * Make a random move.
   *
   * @param random The source of randomness.
   * @return the number of variables that were modified
   */
  virtual size_t randomMove(RandomProvider& random,
                            IAssignment& assignment) = 0;

  /**
   * @return The search variables covered by this neighbourhood.
   */
  [[nodiscard]] virtual const std::vector<SearchVar>& coveredVars() const = 0;

  virtual void commitIf(const IAssignment&) {};
};

}  // namespace atlantis::search::neighbourhoods
