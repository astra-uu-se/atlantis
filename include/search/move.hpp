#pragma once

#include <iostream>
#include <utility>

#include "core/propagationEngine.hpp"

namespace search {

class Move {
 private:
  std::vector<VarId> _targets;
  std::vector<Int> _values;

 public:
  Move(VarId target, Int value)
      : Move(std::vector<VarId>{target}, std::vector<Int>{value}) {}

  Move(std::vector<VarId> targets, std::vector<Int> values)
      : _targets(std::move(targets)), _values(std::move(values)) {}

  void apply(PropagationEngine& engine);

  [[nodiscard]] const std::vector<VarId>& targets() const noexcept {
    return _targets;
  }

  [[nodiscard]] const std::vector<Int>& values() const noexcept {
    return _values;
  }
};

}  // namespace search