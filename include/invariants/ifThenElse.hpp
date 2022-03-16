#pragma once

#include <array>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for z <- if b = 0 then x else y
 *
 */

class IfThenElse : public Invariant {
 private:
  const VarId _b;
  const std::array<const VarId, 2> _xy;
  const VarId _z;

 public:
  IfThenElse(VarId b, VarId x, VarId y, VarId z);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};