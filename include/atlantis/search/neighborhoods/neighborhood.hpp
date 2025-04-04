#pragma once

#include <vector>

namespace atlantis::search {
class SearchVar;
class IAssignment;
class RandomProvider;
}  // namespace atlantis::search

namespace atlantis::search::neighborhoods {

class Neighborhood {
 public:
  virtual ~Neighborhood() = default;

  /**
   * Initialize an assignment.
   *
   * @param random The source of randomness.
   * @param assignment to modify and initialize
   */
  virtual void initialize(RandomProvider& random, IAssignment& assignment) = 0;

  /**
   * Make a random move.
   *
   * @param random The source of randomness.
   * @param assignment the assignment to modify
   * @return the number of variables that were modified
   */
  virtual size_t randomMove(RandomProvider& random,
                            IAssignment& assignment) = 0;

  /**
   * @return The search variables covered by this neighborhood.
   */
  [[nodiscard]] virtual const std::vector<SearchVar>& coveredVars() const = 0;

  virtual void commitIf(const IAssignment&) {}
};

}  // namespace atlantis::search::neighborhoods
