#pragma once

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
  explicit BoolOr(VarId output, VarId x, VarId y);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
