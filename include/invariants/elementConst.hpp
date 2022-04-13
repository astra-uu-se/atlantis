#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;
class Invariant;

/**
 * Invariant for y <- array[index] where array is a vector of constants.
 * NOTE: the index set is 1 based (first element is array[1], not array[0])
 *
 */

class ElementConst : public Invariant {
 private:
  const VarId _index;
  const std::vector<Int> _array;
  const VarId _y;

  [[nodiscard]] inline size_t safeIndex(Int index) noexcept {
    return std::max(Int(1),
                    std::min(static_cast<Int>(_array.size()) - Int(1), index));
  }

  [[nodiscard]] inline std::vector<Int>& prependZero(std::vector<Int>& array) {
    array.emplace_back(0);
    std::rotate(array.rbegin(), array.rbegin() + 1, array.rend());
    return array;
  }

 public:
  ElementConst(VarId index, std::vector<Int> array, VarId y);

  void registerVars(Engine&) override;
  void updateBounds(Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};