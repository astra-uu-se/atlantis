#pragma once

#include <cassert>
#include <limits>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/variables/committable.hpp"
#include "atlantis/propagation/variables/intVar.hpp"
#include "atlantis/propagation/views/intView.hpp"
#include "atlantis/types.hpp"

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
