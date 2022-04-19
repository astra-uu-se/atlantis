#pragma once

#include "assignment.hpp"
#include "cost.hpp"

namespace search {

template <unsigned int N>
class Move {
 public:
  Move(std::array<VarId, N> variables, std::array<Int, N> values)
      : _variables(std::move(variables)), _values(std::move(values)) {}

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
          modifier.set(_variables[i], _values[i]);
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
        modifier.set(_variables[i], _values[i]);
      }
    });
  }

 private:
  std::array<VarId, N> _variables;
  std::array<Int, N> _values;

  Cost _cost{0, 0, Cost::ObjectiveDirection::NONE};
  bool _probed{false};
};

}  // namespace search
