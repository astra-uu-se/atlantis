#pragma once

#include "atlantis/propagation/types.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

template <unsigned int N>
class Move {
 public:
  Move(std::array<propagation::VarId, N> vars, std::array<Int, N> values)
      : _vars(std::move(vars)), _values(std::move(values)) {}

  /**
   * Probe the cost of this move on the given assignment. Will only probe the
   * assignment once.
   *
   * @param assignment The assignment to probe on.
   * @return The cost of the assignment if this move were committed.
   */
  const Cost& probe(const Assignment& assignment) {
    if (!_probed) {
      _cost = assignment.probe([&](auto& modifier) {
        for (auto i = 0u; i < N; i++) {
          modifier.set(_vars[i], _values[i]);
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
    assignment.assign([&](auto& modifier) {
      for (auto i = 0u; i < N; i++) {
        modifier.set(_vars[i], _values[i]);
      }
    });
  }

 private:
  std::array<propagation::VarId, N> _vars;
  std::array<Int, N> _values;

  Cost _cost{0, 0, propagation::ObjectiveDirection::NONE};
  bool _probed{false};
};

}  // namespace atlantis::search
