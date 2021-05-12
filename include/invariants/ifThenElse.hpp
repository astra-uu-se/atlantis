#pragma once

#include <array>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"

// NOTE:
// This is Python syntax and does not come naturally to me
// Seeing IfThenElse(x1, x2, x3, x4) I would assume that
// It would represent x4 = (if x1 then x2 else x3) (as the name of
// invariant suggests). I strongly propone reordering of parameter
// variables
/**
 * Invariant for z <- if b = 0 then x else y
 *
 */

class IfThenElse : public Invariant {
 private:
  VarId _b;
  std::array<VarId, 2> _xy;
  VarId _z;

 public:
  IfThenElse(VarId b, VarId x, VarId y, VarId z);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  VarId getNextParameter(Timestamp, Engine&) override;
  void notifyCurrentParameterChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};