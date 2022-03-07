#pragma once

namespace search {

class Cost;
class Move;
class Neighbourhood;

class Assignment {
 public:
  /**
   * Initialises all the decision variables. The given neighbourhood should
   * define all decision variables at initialisation.
   *
   * @param neighbourhood The neighbourhood used to initialise all the decision
   * variables.
   */
  void initialise(Neighbourhood* neighbourhood);

  /**
   * Determine the cost of a move. Will mutate @p move to indicate it has been
   * probed.
   *
   * @param move The move to probe.
   */
  void probeMove(Move& move);

  /**
   * Change the assignment according to @p move. This will be reflected in
   * the cost of the assignment.
   *
   * @param move The move to commit.
   */
  void commitMove(const Move& move);

  /**
   * @return True if the current assignment satisfies all the constraints, false
   * otherwise.
   */
  bool satisfiesConstraints() const noexcept;

  /**
   * @return The cost of the current assignment.
   */
  Cost cost() const noexcept;
};

}  // namespace search