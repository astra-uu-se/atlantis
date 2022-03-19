#pragma once

#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for y <- varArray[index] where varArray is a vector of VarId.
 * NOTE: the index set is 1 based (first element is varArray[1], not
 * varArray[0])
 *
 */

class ElementVar : public Invariant {
 private:
  const VarId _index;
  const std::vector<VarId> _varArray;
  const VarId _y;

  [[nodiscard]] inline size_t toZeroIndex(Int oneIndex) noexcept {
    return std::max(
        Int(0),
        std::min(static_cast<Int>(_varArray.size()), oneIndex) - Int(1));
  }

 public:
  ElementVar(VarId index, std::vector<VarId> varArray, VarId y);

  void registerVars(Engine&) override;
  void updateBounds(Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};