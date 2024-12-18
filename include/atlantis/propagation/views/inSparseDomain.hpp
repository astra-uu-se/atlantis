#pragma once

#include <vector>

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

class InSparseDomain : public IntView {
  Int _offset;
  std::vector<Int> _valueViolation;

 public:
  explicit InSparseDomain(SolverBase& solver, VarViewId parentId,
                          const std::vector<DomainEntry>& domain);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
