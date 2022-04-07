#pragma once
#include <cmath>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for y <- a % b (integer division)
 *
 */
class Mod : public Invariant {
 private:
  VarId _a, _b, _y;
  Int _zeroReplacement{1};
  Int mod(Int, Int);

 public:
  Mod(VarId a, VarId b, VarId y);
  void registerVars(Engine&) override;
  void updateBounds(Engine&) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};

inline Int Mod::mod(Int aVal, Int bVal) { return aVal % std::abs(bVal); }