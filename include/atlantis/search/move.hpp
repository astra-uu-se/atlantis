#pragma once

#include "atlantis/propagation/types.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

class Move {
 private:
  std::vector<std::pair<propagation::VarId, Int>> _assignment;

  Cost _cost{0, 0, propagation::ObjectiveDirection::NONE};
  bool _probed{false};

 public:
  Move(std::vector<std::pair<propagation::VarId, Int>>&& assignment)
      : _assignment(std::move(assignment)) {}

  /**
   * Probe the cost of this move on the given assignment. Will only probe the
   * assignment once.
   *
   * @param assignment The assignment to probe on.
   * @return The cost of the assignment if this move were committed.
   */
  const Cost& probe(Assignment& assignment) {
    if (!_probed) {
      _cost = assignment.probe([&]([[maybe_unused]] auto& modifier) {
        for (const auto& [var, val] : _assignment) {
          assignment.set(var, val);
        }
      });

      _probed = true;
    }

    return _cost;
  }

  /**
   * Commit this move on the given assignment.
   *
   * @param assignment The assignment to change.
   */
  void commit(Assignment& assignment) {
    assignment.assign([&]([[maybe_unused]] auto& modifier) {
      for (const auto& [var, val] : _assignment) {
        assignment.set(var, val);
      }
    });
  }
};

}  // namespace atlantis::search
