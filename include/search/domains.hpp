#pragma once

#include <variant>

#include "core/types.hpp"
#include "randomProvider.hpp"

namespace search {

class IntervalDomain {
 private:
  Int _lb;
  Int _ub;

 public:
  IntervalDomain(Int lb, Int ub) : _lb(lb), _ub(ub) {}

  [[nodiscard]] Int lowerBound() const noexcept { return _lb; }
  [[nodiscard]] Int upperBound() const noexcept { return _ub; }
};

class SetDomain {
 private:
  std::vector<Int> _values;

 public:
  explicit SetDomain(std::vector<Int> values) : _values(std::move(values)) {}

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
  [[nodiscard]] Int lowerBound() const noexcept { return _values.front(); }
  [[nodiscard]] Int upperBound() const noexcept { return _values.back(); }
};

using SearchDomain = std::variant<IntervalDomain, SetDomain>;

}  // namespace search