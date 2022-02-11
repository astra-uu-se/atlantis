#pragma once

#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for y <- sum(coeffs_i*varArray_i)
 *
 */

class Linear : public Invariant {
 private:
  const std::vector<Int> _coeffs;
  const std::vector<VarId> _varArray;
  std::vector<SavedInt> _localVarArray;
  const VarId _y;

 public:
  Linear(const std::vector<VarId>& varArray, VarId y)
      : Linear(std::vector<Int>(varArray.size(), 1), varArray, y) {}
  Linear(std::vector<Int> coeffs, std::vector<VarId> varArray, VarId y);

#ifndef CBLS_TEST
  void init(Timestamp, Engine&) final;
  void recompute(Timestamp, Engine&) final;
  void notifyIntChanged(Timestamp, Engine&, LocalId) final;
  void commit(Timestamp, Engine&) final;
  VarId getNextInput(Timestamp, Engine&) final;
  void notifyCurrentInputChanged(Timestamp, Engine&) final;
#else
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
#endif
};
