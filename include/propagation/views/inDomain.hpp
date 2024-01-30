#pragma once

#include <cassert>
#include <limits>

#include "propagation/solver.hpp"
#include "propagation/variables/committable.hpp"
#include "propagation/variables/intVar.hpp"
#include "propagation/views/intView.hpp"
#include "types.hpp"

namespace atlantis::propagation {

class SolverBase;

class InDomain : public IntView {
 private:
  std::vector<DomainEntry> _domain;
  VarId _x;
  Committable<std::pair<Int, Int>> _cache;

  [[nodiscard]] Int compute(Int val) const;

 public:
  explicit InDomain(SolverBase& solver, VarId parentId,
                    std::vector<DomainEntry>&& domain);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation