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

  /**
   * @return The number of values of the domain.
   */
  [[nodiscard]] virtual size_t size() const noexcept = 0;

  /**
   * @return if the domain is not a superset of lb..ub,
   * then returns the relative complement of lb..ub in domain,
   * otherwise an empty vector is returned.
   */
  [[nodiscard]] virtual std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const = 0;
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
  [[nodiscard]] size_t size() const noexcept override { return _ub - _lb + 1; }

  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const override {
    if (_lb <= lb && ub <= _ub) {
      return std::vector<DomainEntry>();
    }
    return std::vector<DomainEntry>{{std::max(_lb, lb), std::min(_ub, ub)}};
  }

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

  [[nodiscard]] size_t size() const noexcept override { return _values.size(); }

  std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const override {
    if (lowerBound() <= lb && ub <= upperBound() &&
        static_cast<Int>(size()) == upperBound() - lowerBound() + 1) {
      return std::vector<DomainEntry>();
    }
    assert(size() > 0);

    std::vector<DomainEntry> ret;
    Int domEntryLb = ub + 1;
    for (size_t i = 0; i < _values.size(); ++i) {
      if (_values[i] < lb) {
        continue;
      } else if (_values[i] > ub) {
        // the remaining values of the domain are outside the range
        if (domEntryLb <= ub) {
          if (lb < domEntryLb || _values[i - 1] < ub) {
            // There exists a current domain entry: add it.
            ret.emplace_back(DomainEntry{domEntryLb, _values[i - 1]});
            domEntryLb = ub + 1;
          }
        }
        break;
      } else if (domEntryLb > ub) {
        // store lowerBound for the current DomainEntry:
        domEntryLb = _values[i];
      } else if (0 < i && lb <= _values[i - 1] &&
                 _values[i] != _values[i - 1] + 1) {
        // There is a hole in the domain in the range lb..ub:
        assert(domEntryLb <= ub);
        ret.emplace_back(DomainEntry{domEntryLb, _values[i - 1]});
        domEntryLb = ub + 1;
      }
    }
    if (domEntryLb <= ub) {
      // There exists a current domain entry
      if (lb < domEntryLb || _values.back() < ub) {
        ret.emplace_back(DomainEntry{domEntryLb, std::min(ub, _values.back())});
      }
    }
    return ret;
  }

  bool operator==(const SetDomain& other) const {
    return _values == other._values;
  }

  bool operator!=(const SetDomain& other) const { return !(*this == other); }
};

using SearchDomain = std::variant<IntervalDomain, SetDomain>;
