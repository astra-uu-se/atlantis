#pragma once

#include <variant>
#include <vector>

#include "types.hpp"

/**
 * Models the domain of a variable. Since this might be a large object managing
 * heap memory, the copy constructor is deleted.
 */
class Domain {
 public:
  virtual ~Domain() = default;

  /**
   * Note: Might allocate a lot of memory, depending on the size of the domain.
   * Use sparingly.
   *
   * @return The values in the domain.
   */
  [[nodiscard]] virtual const std::vector<Int>& values() = 0;

  /**
   * @return The value of the smallest element in the domain.
   */
  [[nodiscard]] virtual Int lowerBound() const noexcept = 0;

  /**
   * @return The value of the largest element in the domain.
   */
  [[nodiscard]] virtual Int upperBound() const noexcept = 0;
};

class IntervalDomain : public Domain {
 private:
  Int _lb;
  Int _ub;

  std::vector<Int> _values;
  bool _valuesCollected{false};

 public:
  IntervalDomain(Int lb, Int ub) : _lb(lb), _ub(ub) {}

  [[nodiscard]] const std::vector<Int>& values() override {
    if (!_valuesCollected) {
      _valuesCollected = true;
      _values.resize(_ub - _lb + 1);
      std::iota(_values.begin(), _values.end(), _lb);
    }

    return _values;
  }

  [[nodiscard]] Int lowerBound() const noexcept override { return _lb; }
  [[nodiscard]] Int upperBound() const noexcept override { return _ub; }

  bool operator==(const IntervalDomain& other) const {
    return other._lb == _lb && other._ub == _ub;
  }

  bool operator!=(const IntervalDomain& other) const {
    return !(*this == other);
  }
};

class SetDomain : public Domain {
 private:
  std::vector<Int> _values;

 public:
  explicit SetDomain(std::vector<Int> values) : _values(std::move(values)) {}

  [[nodiscard]] const std::vector<Int>& values() override { return _values; }

  [[nodiscard]] Int lowerBound() const noexcept override {
    return _values.front();
  }

  [[nodiscard]] Int upperBound() const noexcept override {
    return _values.back();
  }

  bool operator==(const SetDomain& other) const {
    return _values == other._values;
  }

  bool operator!=(const SetDomain& other) const {
    return !(*this == other);
  }
};

using SearchDomain = std::variant<IntervalDomain, SetDomain>;
