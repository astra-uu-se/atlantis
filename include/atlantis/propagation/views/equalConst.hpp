#pragma once

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/views/intView.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class EqualConst : public IntView {
 private:
  const Int _val;

 public:
  explicit EqualConst(SolverBase& solver, VarId parentId, Int val);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
