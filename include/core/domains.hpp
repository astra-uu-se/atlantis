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
   * @return The value of the smallest element in the domain.
   */
  [[nodiscard]] virtual Int lowerBound() const noexcept = 0;

  /**
   * @return The value of the largest element in the domain.
   */
  [[nodiscard]] virtual Int upperBound() const noexcept = 0;

  /**
   * @return The values of the lowest and largest elements in the domain.
   */
  [[nodiscard]] virtual std::pair<Int, Int> bounds() const noexcept = 0;

  /**
   * @return The number of values of the domain.
   */
  [[nodiscard]] virtual size_t size() const noexcept = 0;

  /**
   * @return true if the domain constains exactly one value, else false.
   */
  [[nodiscard]] bool isFixed() const noexcept { return size() == 1; };

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

  [[nodiscard]] Int lowerBound() const noexcept override { return _lb; }
  [[nodiscard]] Int upperBound() const noexcept override { return _ub; }
  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override {
    return std::pair<Int, Int>{_lb, _ub};
  }
  [[nodiscard]] size_t size() const noexcept override { return _ub - _lb + 1; }

  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const override {
    if (_lb <= lb && ub <= _ub) {
      return std::vector<DomainEntry>();
    }
    return std::vector<DomainEntry>{{std::max(_lb, lb), std::min(_ub, ub)}};
  }

  void setLowerBound(Int lb) {
    assert(lb <= _ub);
    _lb = lb;
  }

  void setUpperBound(Int ub) {
    assert(_lb <= ub);
    _ub = ub;
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
  explicit SetDomain(std::vector<Int> values) : _values(std::move(values)) {
    std::sort(_values.begin(), _values.end());
  }

  [[nodiscard]] const std::vector<Int>& values() const { return _values; }

  [[nodiscard]] Int lowerBound() const noexcept override {
    return _values.front();
  }

  [[nodiscard]] Int upperBound() const noexcept override {
    return _values.back();
  }

  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override {
    return std::pair<Int, Int>{_values.front(), _values.back()};
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
    // domEntryLb: the lb of the current DomainEntry (ub + 1 is a dummy value)
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

  void removeValue(Int value) {
    if (value < lowerBound() || upperBound() < value) {
      return;
    }
    auto it = std::find(_values.begin(), _values.end(), value);
    if (it != _values.end()) {
      _values.erase(it);
    }
  }

  bool operator==(const SetDomain& other) const {
    return _values == other._values;
  }

  bool operator!=(const SetDomain& other) const { return !(*this == other); }
};

class SearchDomain : public Domain {
 private:
  std::variant<IntervalDomain, SetDomain> _domain;

 public:
  explicit SearchDomain(std::vector<Int> values) : _domain(SetDomain{values}) {}
  explicit SearchDomain(Int lb, Int ub) : _domain(IntervalDomain{lb, ub}) {}

  [[nodiscard]] inline const std::variant<IntervalDomain, SetDomain>&
  innerDomain() const noexcept {
    return _domain;
  }

  [[nodiscard]] const std::vector<Int>& values() {
    if (std::holds_alternative<IntervalDomain>(_domain)) {
      std::vector<Int> values(upperBound() - lowerBound() + 1);
      std::iota(values.begin(), values.end(), lowerBound());
      _domain = SetDomain(values);
    }
    assert(std::holds_alternative<SetDomain>(_domain));
    return std::get<SetDomain>(_domain).values();
  }

  [[nodiscard]] Int lowerBound() const noexcept override {
    return std::visit<Int>([&](const auto& dom) { return dom.lowerBound(); },
                           _domain);
  }

  [[nodiscard]] Int upperBound() const noexcept override {
    return std::visit<Int>([&](const auto& dom) { return dom.upperBound(); },
                           _domain);
  }

  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override {
    return std::visit<std::pair<Int, Int>>(
        [&](const auto& dom) { return dom.bounds(); }, _domain);
  }

  [[nodiscard]] size_t size() const noexcept override {
    return std::visit<Int>([&](const auto& dom) { return dom.size(); },
                           _domain);
  }

  std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const override {
    return std::visit<std::vector<DomainEntry>>(
        [&](const auto& dom) {
          return dom.relativeComplementIfIntersects(lb, ub);
        },
        _domain);
  }

  void removeValue(Int value) {
    // do nothing if the value is not in the domain:
    if (value < lowerBound() || upperBound() < value) {
      return;
    }
    if (std::holds_alternative<SetDomain>(_domain)) {
      // Remove the value from the set domain:
      std::get<SetDomain>(_domain).removeValue(value);
      return;
    }
    assert(std::holds_alternative<IntervalDomain>(_domain));
    if (value == lowerBound()) {
      // change lb
      std::get<IntervalDomain>(_domain).setLowerBound(value + 1);
      return;
    } else if (value == upperBound()) {
      // change ub
      std::get<IntervalDomain>(_domain).setUpperBound(value - 1);
      return;
    }
    std::vector<Int> newDomain(upperBound() - lowerBound());
    Int i = 0;
    for (Int val = lowerBound(); val <= upperBound(); ++val) {
      if (val != value) {
        newDomain.at(i) = val;
        ++i;
      }
    }
    _domain = SetDomain(newDomain);
  }

  bool operator==(const SearchDomain& other) const {
    if (std::holds_alternative<IntervalDomain>(_domain) &&
        std::holds_alternative<IntervalDomain>(other._domain)) {
      return std::get<IntervalDomain>(_domain) ==
             std::get<IntervalDomain>(other._domain);
    }
    if (std::holds_alternative<SetDomain>(_domain) &&
        std::holds_alternative<SetDomain>(other._domain)) {
      return std::get<SetDomain>(_domain) == std::get<SetDomain>(other._domain);
    }
    return false;
  }

  bool operator!=(const SearchDomain& other) const {
    if (std::holds_alternative<IntervalDomain>(_domain) &&
        std::holds_alternative<IntervalDomain>(other._domain)) {
      return std::get<IntervalDomain>(_domain) !=
             std::get<IntervalDomain>(other._domain);
    }
    if (std::holds_alternative<SetDomain>(_domain) &&
        std::holds_alternative<SetDomain>(other._domain)) {
      return std::get<SetDomain>(_domain) != std::get<SetDomain>(other._domain);
    }
    return true;
  }
};
