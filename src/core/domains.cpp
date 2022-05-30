#include "core/domains.hpp"

#include <numeric>

Int IntervalDomain::lowerBound() const noexcept { return _lb; }
Int IntervalDomain::upperBound() const noexcept { return _ub; }

std::pair<Int, Int> IntervalDomain::bounds() const noexcept {
  return std::pair<Int, Int>{_lb, _ub};
}

size_t IntervalDomain::size() const noexcept { return _ub - _lb + 1; }

std::vector<DomainEntry> IntervalDomain::relativeComplementIfIntersects(
    const Int lb, const Int ub) const {
  if (_lb <= lb && ub <= _ub) {
    return {};
  }

  return std::vector<DomainEntry>{{std::max(_lb, lb), std::min(_ub, ub)}};
}

void IntervalDomain::setLowerBound(Int lb) {
  assert(lb <= _ub);
  _lb = lb;
}

void IntervalDomain::setUpperBound(Int ub) {
  assert(_lb <= ub);
  _ub = ub;
}

bool IntervalDomain::operator==(const IntervalDomain& other) const {
  return other._lb == _lb && other._ub == _ub;
}

bool IntervalDomain::operator!=(const IntervalDomain& other) const {
  return !(*this == other);
}

SetDomain::SetDomain(std::vector<Int> values) : _values(std::move(values)) {
  std::sort(_values.begin(), _values.end());
}

Int SetDomain::lowerBound() const noexcept { return _values.front(); }

Int SetDomain::upperBound() const noexcept { return _values.back(); }

std::pair<Int, Int> SetDomain::bounds() const noexcept {
  return std::pair<Int, Int>{_values.front(), _values.back()};
}

size_t SetDomain::size() const noexcept { return _values.size(); }

std::vector<DomainEntry> SetDomain::relativeComplementIfIntersects(
    const Int lb, const Int ub) const {
  if (lowerBound() <= lb && ub <= upperBound() &&
      static_cast<Int>(size()) == upperBound() - lowerBound() + 1) {
    return {};
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

void SetDomain::removeValue(Int value) {
  if (value < lowerBound() || upperBound() < value) {
    return;
  }
  auto it = std::find(_values.begin(), _values.end(), value);
  if (it != _values.end()) {
    _values.erase(it);
  }
}

bool SetDomain::operator==(const SetDomain& other) const {
  return _values == other._values;
}

bool SetDomain::operator!=(const SetDomain& other) const {
  return !(*this == other);
}

const std::variant<IntervalDomain, SetDomain>& SearchDomain::innerDomain()
    const noexcept {
  return _domain;
}

const std::vector<Int>& SearchDomain::values() {
  if (std::holds_alternative<IntervalDomain>(_domain)) {
    std::vector<Int> values(upperBound() - lowerBound() + 1);
    std::iota(values.begin(), values.end(), lowerBound());
    _domain = SetDomain(values);
  }
  assert(std::holds_alternative<SetDomain>(_domain));
  return std::get<SetDomain>(_domain).values();
}

Int SearchDomain::lowerBound() const noexcept {
  return std::visit<Int>([&](const auto& dom) { return dom.lowerBound(); },
                         _domain);
}

Int SearchDomain::upperBound() const noexcept {
  return std::visit<Int>([&](const auto& dom) { return dom.upperBound(); },
                         _domain);
}

std::pair<Int, Int> SearchDomain::bounds() const noexcept {
  return std::visit<std::pair<Int, Int>>(
      [&](const auto& dom) { return dom.bounds(); }, _domain);
}

size_t SearchDomain::size() const noexcept {
  return std::visit<Int>([&](const auto& dom) { return dom.size(); }, _domain);
}

std::vector<DomainEntry> SearchDomain::relativeComplementIfIntersects(
    const Int lb, const Int ub) const {
  return std::visit<std::vector<DomainEntry>>(
      [&](const auto& dom) {
        return dom.relativeComplementIfIntersects(lb, ub);
      },
      _domain);
}

void SearchDomain::removeValue(Int value) {
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

bool SearchDomain::operator==(const SearchDomain& other) const {
  return lowerBound() == other.lowerBound() &&
         upperBound() == other.upperBound() && size() == other.size();
}

bool SearchDomain::operator!=(const SearchDomain& other) const {
  return !(*this == other);
}
