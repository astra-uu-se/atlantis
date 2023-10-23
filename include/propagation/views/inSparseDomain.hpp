#pragma once

#include <cassert>
#include <limits>

#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/variables/intVar.hpp"
#include "propagation/views/intView.hpp"

namespace atlantis::propagation {

class Engine;

class InSparseDomain : public IntView {
 private:
  const Int _offset;
  std::vector<int> _valueViolation;

 public:
  explicit InSparseDomain(Engine& engine, VarId parentId,
                          const std::vector<DomainEntry>& domain);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation