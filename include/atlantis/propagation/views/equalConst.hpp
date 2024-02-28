#pragma once

#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {

class EqualConst : public IntView {
 private:
  const Int _val;

 public:
  explicit EqualConst(SolverBase& solver, VarId parentId, Int val)
      : IntView(solver, parentId), _val(val) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
