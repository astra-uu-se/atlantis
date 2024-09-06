#pragma once

#include <stdexcept>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {

class ExpView : public IntView {
 private:
  Int _power;

 protected:
  static inline Int pow(Int base, Int power) {
    if (power == 0) {
      return 1;
    }
    if (power == 1) {
      return base;
    }
    if (power < 0) {
      if (base == 0) {
        throw std::runtime_error("negative power of zero");
      }
      if (base == 1) {
        return 1;
      }
      if (base == -1) {
        return power % 2 == 0 ? 1 : -1;
      }
      return 0;
    }
    Int result = 1;
    for (int i = 0; i < power; i++) {
      result *= base;
    }
    return result;
  }

 public:
  explicit ExpView(SolverBase& solver, VarViewId parentId, Int power)
      : IntView(solver, parentId), _power(power) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
