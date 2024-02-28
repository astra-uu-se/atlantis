#pragma once

#include <cassert>
#include <limits>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/variables/intVar.hpp"
#include "atlantis/propagation/views/intView.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class InSparseDomain : public IntView {
 private:
  Int _offset;
  std::vector<Int> _valueViolation;

 public:
  explicit InSparseDomain(SolverBase& solver, VarId parentId,
                          const std::vector<DomainEntry>& domain);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
