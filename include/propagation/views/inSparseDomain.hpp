#pragma once

#include <cassert>
#include <limits>

#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/variables/intVar.hpp"
#include "propagation/views/intView.hpp"

namespace atlantis::propagation {

class SolverBase;

class InSparseDomain : public IntView {
 private:
  const Int _offset;
  std::vector<int> _valueViolation;

 public:
  explicit InSparseDomain(SolverBase& solver, VarId parentId,
                          const std::vector<DomainEntry>& domain);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation