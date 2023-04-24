#pragma once

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"
#include "variables/intVar.hpp"

class Engine;

/**
 * Invariant for output <- x \/ y
 *
 */
class BoolOr : public Invariant {
 private:
  const VarId _output, _x, _y;

 public:
  explicit BoolOr(Engine&, VarId output, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};
