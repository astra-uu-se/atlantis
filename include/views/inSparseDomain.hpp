#pragma once

#include <cassert>
#include <limits>

#include "core/types.hpp"
#include "variables/intVar.hpp"
#include "views/intView.hpp"

class Engine;
class InSparseDomain : public IntView {
 private:
  const Int _offset;
  std::vector<int> _valueViolation;

 public:
  explicit InSparseDomain(VarId parentId,
                          const std::vector<DomainEntry>& domain);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};
