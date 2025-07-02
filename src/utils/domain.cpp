#include <algorithm>
#include <chrono>
#include <numeric>
#include <stdexcept>
#include <string>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis {

static bool sortedVectorIsInterval(const std::vector<Int>& sortedVals) {
  return !sortedVals.empty() &&
         sortedVals.front() + static_cast<Int>(sortedVals.size()) - 1 ==
             sortedVals.back();
}

static bool isSubsetOf(const std::vector<Int>& subset,
                       const std::vector<Int>& superset) {
  assert(std::ranges::adjacent_find(subset, std::greater_equal<>()) ==
         subset.end());
  assert(std::ranges::adjacent_find(superset, std::greater_equal<>()) ==
         superset.end());
  if (subset.empty()) {
    return true;
  }

  if (superset.empty()) {
    return false;
  }

  return std::ranges::includes(superset, subset);
}

static bool isSubsetOf(Int subsetLb, Int subsetUb,
                       const std::vector<Int>& superset) {
  assert(subsetLb <= subsetUb);
  assert(std::ranges::adjacent_find(superset, std::greater_equal<>()) ==
         superset.end());
  if (superset.empty()) {
    return false;
  }

  if (subsetLb < superset.front() || superset.back() < subsetUb) {
    return false;
  }

  auto iter = std::ranges::find_if(superset,
                                   [&](const Int v) { return v == subsetLb; });
  if (iter == superset.end()) {
    return false;
  }
  Int last = *iter;
  for (++iter; iter != superset.end(); ++iter) {
    if (++last != *iter) {
      return false;
    }
  }
  return true;
}

static bool isSubsetOf(const std::vector<Int>& subset, Int supersetLb,
                       Int supersetUb) {
  assert(std::ranges::adjacent_find(subset, std::greater_equal<>()) ==
         subset.end());
  assert(supersetLb <= supersetUb);
  if (subset.empty()) {
    return true;
  }

  return supersetLb <= subset.front() && subset.back() <= supersetUb &&
         sortedVectorIsInterval(subset);
}

static bool isSubsetOf(Int subsetLb, Int subsetUb, Int supersetLb,
                       Int supersetUb) {
  assert(subsetLb <= subsetUb);
  assert(supersetLb <= supersetUb);
  return supersetLb <= subsetLb && subsetUb <= supersetUb;
}

Domain::Iterator::Iterator(Int lb, Int ub, Int pos)
    : _data(std::pair<Int, Int>(lb, ub + 1)), _pos{pos} {}

Domain::Iterator::Iterator(const std::vector<Int>& vals, size_t pos)
    : _data(&vals), _pos{static_cast<Int>(pos)} {}

const Int& Domain::Iterator::operator*() const {
  return std::holds_alternative<std::pair<Int, Int>>(_data)
             ? _pos
             : (std::get<std::vector<Int> const*>(_data)->operator[](_pos));
}

Int const* Domain::Iterator::operator->() const {
  return std::holds_alternative<std::pair<Int, Int>>(_data)
             ? &_pos
             : &(std::get<std::vector<Int> const*>(_data)->operator[](_pos));
}

Domain::Iterator& Domain::Iterator::operator++() {
  ++_pos;
  return *this;
}

Domain::Iterator Domain::Iterator::operator++(int) {
  Iterator tmp = *this;
  ++_pos;
  return tmp;
}

Domain::Iterator& Domain::Iterator::operator--() {
  --_pos;
  return *this;
}

Domain::Iterator Domain::Iterator::operator--(int) {
  Iterator tmp = *this;
  --_pos;
  return tmp;
}

Domain::Iterator& Domain::Iterator::operator+=(size_t offset) {
  _pos += static_cast<Int>(offset);
  return *this;
}

Domain::Iterator& Domain::Iterator::operator-=(size_t offset) {
  _pos -= static_cast<Int>(offset);
  return *this;
}

Domain::Iterator Domain::Iterator::operator+(size_t offset) const {
  Iterator tmp = *this;
  tmp._pos += static_cast<Int>(offset);
  return tmp;
}

Domain::Iterator Domain::Iterator::operator-(size_t offset) const {
  Iterator tmp = *this;
  tmp._pos -= static_cast<Int>(offset);
  return tmp;
}

bool Domain::Iterator::operator==(const Iterator& other) const {
  if (std::holds_alternative<std::pair<Int, Int>>(_data) !=
          std::holds_alternative<std::pair<Int, Int>>(other._data) ||
      _pos != other._pos) {
    return false;
  }
  if (std::holds_alternative<std::pair<Int, Int>>(_data)) {
    return std::get<std::pair<Int, Int>>(_data) ==
           std::get<std::pair<Int, Int>>(other._data);
  }
  return std::get<std::vector<Int> const*>(_data) ==
         std::get<std::vector<Int> const*>(other._data);
}

bool Domain::Iterator::operator!=(const Iterator& other) const {
  return !operator==(other);
}

IntervalDomain::IntervalDomain(Int lb, Int ub) : _lb(lb), _ub(ub) {
  if (lb > ub) {
    throw InconsistencyException(
        "InterValDomain::InterValDomain: " + std::to_string(lb) + " > " +
        std::to_string(ub));
  }
}

Int IntervalDomain::lowerBound() const { return _lb; }

Int IntervalDomain::upperBound() const { return _ub; }

std::pair<Int, Int> IntervalDomain::bounds() const {
  return std::pair<Int, Int>{_lb, _ub};
}

size_t IntervalDomain::size() const noexcept { return _ub - _lb + 1; }

bool IntervalDomain::isFixed() const noexcept { return _lb == _ub; }

bool IntervalDomain::contains(Int value) const noexcept {
  return _lb <= value && value <= _ub;
}

bool IntervalDomain::contains(Int lb, Int ub) const noexcept {
  return isSubsetOf(lb, ub, _lb, _ub);
}

bool IntervalDomain::contains(const IntervalDomain& other) const noexcept {
  return isSubsetOf(other._lb, other._ub, _lb, _ub);
}

bool IntervalDomain::contains(const SetDomain& other) const noexcept {
  return isSubsetOf(other.values(), _lb, _ub);
}

bool IntervalDomain::contains(const SortedUniqueVector& vals) const noexcept {
  return isSubsetOf(*vals, _lb, _ub);
}

bool IntervalDomain::isContained(const IntervalDomain& other) const {
  return isSubsetOf(_lb, _ub, other._lb, other._ub);
}

bool IntervalDomain::isContained(const SetDomain& other) const {
  return isSubsetOf(_lb, _ub, other.values());
}

bool IntervalDomain::isContained(Int lb, Int ub) const {
  return isSubsetOf(_lb, _ub, lb, ub);
}

bool IntervalDomain::isContained(const SortedUniqueVector& vals) const {
  return isSubsetOf(_lb, _ub, *vals);
}

bool IntervalDomain::isInterval() const noexcept { return true; }

std::vector<DomainEntry> IntervalDomain::createDomainEntries(
    const Int lb, const Int ub) const {
  if (_lb <= lb && ub <= _ub) {
    return {};
  }
  return std::vector<DomainEntry>{{std::max(_lb, lb), std::min(_ub, ub)}};
}

Domain::Iterator IntervalDomain::begin() const {
  return Iterator(_lb, _ub, _lb);
}

Domain::Iterator IntervalDomain::end() const {
  return Iterator(_lb, _ub, _ub + 1);
}

Int IntervalDomain::at(size_t offset) const {
  assert(_lb + static_cast<Int>(offset) <= _ub);
  return _lb + static_cast<Int>(offset);
}

Int IntervalDomain::operator[](size_t offset) const { return at(offset); }

void IntervalDomain::setLowerBound(Int lb) {
  if (lb > _ub) {
    throw InconsistencyException(
        "IntervalDomain::setLowerBound: " + std::to_string(lb) + " > " +
        std::to_string(_ub));
  }
  _lb = lb;
}

void IntervalDomain::setUpperBound(Int ub) {
  if (_lb > ub) {
    throw InconsistencyException(
        "InterValDomain::setUpperBound: " + std::to_string(_lb) + " > " +
        std::to_string(ub));
  }
  _ub = ub;
}

bool IntervalDomain::isDisjoint(Int lb, Int ub) const {
  return ub < _lb || _ub < lb;
}

bool IntervalDomain::isDisjoint(const IntervalDomain& other) const {
  return isDisjoint(other._lb, other._ub);
}

bool IntervalDomain::isDisjoint(const SortedUniqueVector& vals) const {
  if ((*vals).empty()) {
    return true;
  }
  return isDisjoint((*vals).front(), (*vals).back());
}

bool IntervalDomain::isDisjoint(const SetDomain& other) const {
  return isDisjoint(other.lowerBound(), other.upperBound());
}

void IntervalDomain::intersect(Int lb, Int ub) {
  _lb = std::max(lb, _lb);
  _ub = std::min(ub, _ub);
  if (_lb > _ub) {
    throw InconsistencyException("IntervalDomain::intersect: Empty domain");
  }
}

void IntervalDomain::fix(Int value) {
  if (!contains(value)) {
    throw InconsistencyException("IntervalDomain::fix: Empty domain");
  }
  _lb = value;
  _ub = value;
}

bool IntervalDomain::isEqual(Int lb, Int ub) const {
  assert(lb <= ub);
  return lb == _lb && ub == _ub;
}

bool IntervalDomain::operator==(const SortedUniqueVector& vals) const {
  if ((*vals).empty() || !vals.isInterval()) {
    return false;
  }
  return isEqual((*vals).front(), (*vals).back());
}

bool IntervalDomain::operator==(const IntervalDomain& other) const {
  return isEqual(other._lb, other._ub);
}

bool IntervalDomain::operator==(const SetDomain& other) const {
  if (other.isInterval()) {
    return false;
  }
  return isEqual(other.lowerBound(), other.upperBound());
}

bool IntervalDomain::operator!=(const SortedUniqueVector& vals) const {
  return !(*this == vals);
}

bool IntervalDomain::operator!=(const IntervalDomain& other) const {
  return !(*this == other);
}

bool IntervalDomain::operator!=(const SetDomain& other) const {
  return !(*this == other);
}

SetDomain::SetDomain(std::vector<Int>&& values) : _values(std::move(values)) {
  if (_values.empty()) {
    throw InconsistencyException("SetDomain::SetDomain: empty domain");
  }
  std::ranges::sort(_values.begin(), _values.end());
  const auto [first, last] = std::ranges::unique(_values);
  _values.erase(first, last);
  assert(!_values.empty());
}

SetDomain::SetDomain(const std::vector<Int>& values)
    : SetDomain(std::vector<Int>(values)) {}

const std::vector<Int>& SetDomain::values() const { return _values; }

Int SetDomain::lowerBound() const { return _values.front(); }

Int SetDomain::upperBound() const { return _values.back(); }

std::pair<Int, Int> SetDomain::bounds() const {
  return std::pair<Int, Int>{_values.front(), _values.back()};
}

size_t SetDomain::size() const noexcept { return _values.size(); }
bool SetDomain::isFixed() const noexcept { return _values.size() == 1; }

bool SetDomain::contains(Int value) const noexcept {
  return std::ranges::binary_search(_values, value);
}

bool SetDomain::contains(Int lb, Int ub) const noexcept {
  return isSubsetOf(lb, ub, _values);
}

bool SetDomain::contains(const IntervalDomain& other) const noexcept {
  return isSubsetOf(other.lowerBound(), other.upperBound(), _values);
}

bool SetDomain::contains(const std::vector<Int>& vals) const noexcept {
  return isSubsetOf(vals, _values);
}

bool SetDomain::contains(const SortedUniqueVector& vals) const noexcept {
  return isSubsetOf(*vals, _values);
}

bool SetDomain::contains(const SetDomain& other) const noexcept {
  return isSubsetOf(other._values, _values);
}

bool SetDomain::isContained(const IntervalDomain& other) const {
  return isSubsetOf(_values, other.lowerBound(), other.upperBound());
}

bool SetDomain::isContained(const SetDomain& other) const {
  return isSubsetOf(_values, other._values);
}

bool SetDomain::isContained(Int lb, Int ub) const {
  return isSubsetOf(_values, lb, ub);
}

bool SetDomain::isContained(const SortedUniqueVector& vals) const {
  return isSubsetOf(_values, *vals);
}

bool SetDomain::isInterval() const noexcept {
  return sortedVectorIsInterval(_values);
}

Domain::Iterator SetDomain::begin() const { return Iterator(_values, 0); }

Domain::Iterator SetDomain::end() const {
  return Iterator(_values, _values.size());
}

Int SetDomain::at(size_t offset) const { return _values.at(offset); }

Int SetDomain::operator[](size_t offset) const { return _values[offset]; }

std::vector<DomainEntry> SetDomain::createDomainEntries(const Int lb,
                                                        const Int ub) const {
  if (ub < lowerBound() || upperBound() < lb) {
    return {};
  }
  std::vector<DomainEntry> ret;

  size_t i = 0;
  for (; i < _values.size(); ++i) {
    if (_values[i] >= lb) {
      break;
    }
  }
  for (; i < _values.size() && _values[i] <= ub; ++i) {
    const Int iLb = _values[i];
    for (; i + 1 < _values.size() && _values[i + 1] <= ub; ++i) {
      if (_values[i] + 1 != _values[i + 1]) {
        break;
      }
    }
    const Int iUb = _values[i];
    ret.emplace_back(iLb, iUb);
  }
  if (ret.size() == 1 && ret.front().lowerBound == lb &&
      ret.front().upperBound == ub) {
    return {};
  }
  return ret;
}

void SetDomain::remove(Int value) {
  if (value < lowerBound() || upperBound() < value) {
    return;
  }
  if (isFixed()) {
    throw InconsistencyException("SetDomain::remove: Empty domain");
  }
  auto it = std::ranges::find(_values.begin(), _values.end(), value);
  if (it != _values.end()) {
    _values.erase(it);
  }
  assert(!_values.empty());
}

void SetDomain::remove(Int lb, Int ub) {
  auto begin = std::ranges::find_if(
      _values, [&](const Int value) { return value >= lb; });
  auto end = std::find_if(begin, _values.end(),
                          [&](const Int value) { return value > ub; });
  _values.erase(begin, end);
  if (_values.empty()) {
    throw InconsistencyException("SetDomain::remove: Empty domain");
  }
}

void SetDomain::remove(const IntervalDomain& other) {
  return remove(other.lowerBound(), other.upperBound());
}

void SetDomain::remove(const std::vector<Int>& vals) {
  size_t i = 0;
  Int j = 0;
  while (i < vals.size() && j < static_cast<Int>(_values.size())) {
    if (vals[i] > _values[j]) {
      ++j;
    } else {
      if (vals[i] == _values[j]) {
        _values.erase(_values.begin() + j);
      }
      ++i;
    }
  }
  if (_values.empty()) {
    throw InconsistencyException("SetDomain::remove: Empty domain");
  }
}

void SetDomain::remove(const SetDomain& other) {
  if (other.isInterval()) {
    return remove(other.lowerBound(), other.upperBound());
  }
  return remove(other._values);
}

void SetDomain::remove(const SortedUniqueVector& values) {
  if ((*values).empty()) {
    return;
  }
  if (values.isInterval()) {
    return remove((*values).front(), (*values).back());
  }
  return remove(*values);
}

void SetDomain::removeBelow(Int newLowerBound) {
  if (newLowerBound <= lowerBound()) {
    return;
  }
  if (upperBound() < newLowerBound) {
    throw InconsistencyException("SetDomain::removeBelow: Empty domain");
  }
  const auto end = std::ranges::find_if(
      _values, [&](Int value) { return value >= newLowerBound; });
  assert(end != _values.begin());
  _values.erase(_values.begin(), end);
  assert(!_values.empty());
}

void SetDomain::removeAbove(Int newUpperBound) {
  if (upperBound() <= newUpperBound) {
    return;
  }
  if (newUpperBound < lowerBound()) {
    throw InconsistencyException("SetDomain::removeAbove: Empty domain");
  }
  const auto begin = std::ranges::find_if(
      _values, [&](const Int val) { return val > newUpperBound; });
  assert(begin != _values.end());
  _values.erase(begin, _values.end());
  assert(!_values.empty());
}

bool SetDomain::isDisjoint(const std::vector<Int>& vals) const {
  assert(std::ranges::adjacent_find(vals, std::greater_equal<>()) ==
         vals.end());
  if (_values.empty()) {
    return true;
  }
  if (vals.front() < lowerBound() || upperBound() < vals.back()) {
    return true;
  }
  size_t i = 0;
  size_t j = 0;
  while (i < _values.size() && j < vals.size()) {
    if (_values[i] < vals[j]) {
      ++i;
    } else if (_values[i] > vals[j]) {
      ++j;
    } else {
      return false;
    }
  }
  return true;
}

bool SetDomain::isDisjoint(const SortedUniqueVector& values) const {
  return isDisjoint(*values);
}

bool SetDomain::isDisjoint(Int lb, Int ub) const {
  assert(lb <= ub);
  if (ub < lowerBound() || upperBound() < lb) {
    return true;
  }
  return std::ranges::none_of(_values,
                              [&](const Int v) { return lb <= v && v <= ub; });
}

bool SetDomain::isDisjoint(const IntervalDomain& other) const {
  return isDisjoint(other.lowerBound(), other.upperBound());
}

bool SetDomain::isDisjoint(const SetDomain& other) const {
  return isDisjoint(other._values);
}

void SetDomain::intersect(const std::vector<Int>& otherVals) {
  assert(std::ranges::adjacent_find(otherVals, std::greater_equal<>()) ==
         otherVals.end());
  std::vector<Int> newValues;
  newValues.reserve(std::min(_values.size(), otherVals.size()));

  std::ranges::set_intersection(_values, otherVals,
                                std::back_inserter(newValues));

  _values = std::move(newValues);
  if (_values.empty()) {
    throw InconsistencyException("SetDomain::intersect: Empty domain");
  }
}

void SetDomain::intersect(const SortedUniqueVector& otherVals) {
  intersect(*otherVals);
}

void SetDomain::intersect(const SetDomain& otherVals) {
  intersect(otherVals._values);
}

void SetDomain::fix(Int value) {
  if (!contains(value)) {
    throw InconsistencyException("SetDomain::fix: Empty domain");
  }
  _values = std::vector<Int>{value};
}

bool SetDomain::isEqual(Int lb, Int ub) const {
  assert(lb <= ub);
  if (!isInterval()) {
    return false;
  }
  return lowerBound() == lb && upperBound() == ub;
}

bool SetDomain::operator==(const SortedUniqueVector& vals) const {
  return (*vals) == _values;
}

bool SetDomain::operator==(const IntervalDomain& other) const {
  return isEqual(other.lowerBound(), other.upperBound());
}

bool SetDomain::operator==(const SetDomain& other) const {
  return _values == other._values;
}

bool SetDomain::operator!=(const SortedUniqueVector& vals) const {
  return !(*this == vals);
}

bool SetDomain::operator!=(const IntervalDomain& other) const {
  return !(*this == other);
}

bool SetDomain::operator!=(const SetDomain& other) const {
  return !(*this == other);
}

SearchDomain::SearchDomain(std::vector<Int>&& values)
    : _domain(SetDomain(std::move(values))) {}

SearchDomain::SearchDomain(const std::vector<Int>& values)
    : _domain(SetDomain(values)) {}

SearchDomain::SearchDomain(Int lb, Int ub) : _domain(IntervalDomain(lb, ub)) {}

const std::variant<IntervalDomain, SetDomain>& SearchDomain::innerDomain()
    const noexcept {
  return _domain;
}

const std::vector<Int>& SearchDomain::values() {
  if (std::holds_alternative<IntervalDomain>(_domain)) {
    std::vector<Int> values(upperBound() - lowerBound() + 1);
    std::iota(values.begin(), values.end(), lowerBound());
    _domain = SetDomain(std::move(values));
  }
  assert(std::holds_alternative<SetDomain>(_domain));
  return std::get<SetDomain>(_domain).values();
}

Int SearchDomain::lowerBound() const {
  return std::visit<Int>([&](const auto& dom) { return dom.lowerBound(); },
                         _domain);
}

Int SearchDomain::upperBound() const {
  return std::visit<Int>([&](const auto& dom) { return dom.upperBound(); },
                         _domain);
}

std::pair<Int, Int> SearchDomain::bounds() const {
  return std::visit<std::pair<Int, Int>>(
      [&](const auto& dom) { return dom.bounds(); }, _domain);
}

size_t SearchDomain::size() const noexcept {
  return std::visit<Int>([&](const auto& dom) { return dom.size(); }, _domain);
}

bool SearchDomain::isFixed() const noexcept {
  return std::visit<bool>([&](const auto& dom) { return dom.isFixed(); },
                          _domain);
}

bool SearchDomain::contains(Int value) const noexcept {
  return std::visit<bool>([&](const auto& dom) { return dom.contains(value); },
                          _domain);
}

bool SearchDomain::contains(Int lb, Int ub) const noexcept {
  return std::visit<bool>([&](const auto& dom) { return dom.contains(lb, ub); },
                          _domain);
}

bool SearchDomain::contains(const SortedUniqueVector& vals) const noexcept {
  return std::visit<bool>([&](const auto& dom) { return dom.contains(vals); },
                          _domain);
}

bool SearchDomain::contains(const IntervalDomain& other) const noexcept {
  return std::visit<bool>([&](const auto& dom) { return dom.contains(other); },
                          _domain);
}

bool SearchDomain::contains(const SetDomain& other) const noexcept {
  return std::visit<bool>([&](const auto& dom) { return dom.contains(other); },
                          _domain);
}

bool SearchDomain::contains(const SearchDomain& other) const noexcept {
  return std::visit<bool>([&](const auto& o) { return this->contains(o); },
                          other._domain);
}

bool SearchDomain::isContained(Int lb, Int ub) const {
  return std::visit<bool>(
      [&](const auto& dom) { return dom.isContained(lb, ub); }, _domain);
}

bool SearchDomain::isContained(const IntervalDomain& other) const {
  return std::visit<bool>(
      [&](const auto& dom) { return dom.isContained(other); }, _domain);
}

bool SearchDomain::isContained(const SetDomain& other) const {
  return std::visit<bool>(
      [&](const auto& dom) { return dom.isContained(other); }, _domain);
}

bool SearchDomain::isContained(const SortedUniqueVector& vals) const {
  return std::visit<bool>(
      [&](const auto& dom) { return dom.isContained(vals); }, _domain);
}

bool SearchDomain::isContained(const SearchDomain& other) const {
  return std::visit<bool>([&](const auto& o) { return this->isContained(o); },
                          other._domain);
}

bool SearchDomain::isInterval() const noexcept {
  return std::visit<bool>([&](const auto& dom) { return dom.isInterval(); },
                          _domain);
}

Domain::Iterator SearchDomain::begin() const {
  return std::visit<Iterator>([&](const auto& dom) { return dom.begin(); },
                              _domain);
}

Domain::Iterator SearchDomain::end() const {
  return std::visit<Iterator>([&](const auto& dom) { return dom.end(); },
                              _domain);
}

Int SearchDomain::at(size_t offset) const {
  return std::visit<Int>([&](const auto& dom) { return dom.at(offset); },
                         _domain);
}

Int SearchDomain::operator[](size_t offset) const {
  return std::visit<Int>([&](const auto& dom) { return dom[offset]; }, _domain);
}

std::vector<DomainEntry> SearchDomain::createDomainEntries(const Int lb,
                                                           const Int ub) const {
  return std::visit<std::vector<DomainEntry>>(
      [&](const auto& dom) { return dom.createDomainEntries(lb, ub); },
      _domain);
}

void SearchDomain::remove(Int value) {
  // do nothing if the value is not in the domain:
  if (value < lowerBound() || upperBound() < value) {
    return;
  }
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the value from the set domain:
    std::get<SetDomain>(_domain).remove(value);
    return;
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  if (value == lowerBound()) {
    // change lb
    std::get<IntervalDomain>(_domain).setLowerBound(value + 1);
    return;
  }
  if (value == upperBound()) {
    // change ub
    std::get<IntervalDomain>(_domain).setUpperBound(value - 1);
    return;
  }
  std::vector<Int> newDomain(upperBound() - lowerBound());
  std::iota(newDomain.begin(), newDomain.begin() + value - lowerBound(),
            lowerBound());
  std::iota(newDomain.begin() + value - lowerBound(), newDomain.end(),
            value + 1);
  _domain = SetDomain(std::move(newDomain));
}

void SearchDomain::removeBelow(Int newLowerBound) {
  if (newLowerBound <= lowerBound()) {
    return;
  }
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the value from the set domain:
    std::get<SetDomain>(_domain).removeBelow(newLowerBound);
    return;
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  std::get<IntervalDomain>(_domain).setLowerBound(newLowerBound);
}

void SearchDomain::removeAbove(Int newUpperBound) {
  if (newUpperBound >= upperBound()) {
    return;
  }
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the value from the set domain:
    std::get<SetDomain>(_domain).removeAbove(newUpperBound);
    return;
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  std::get<IntervalDomain>(_domain).setUpperBound(newUpperBound);
}

void SearchDomain::remove(const std::vector<Int>& vals) {
  assert(std::holds_alternative<IntervalDomain>(_domain));
  if (vals.empty()) {
    return;
  }
  if (sortedVectorIsInterval(vals)) {
    return remove(vals.front(), vals.back());
  }
  for (const Int value : vals) {
    remove(value);
  }
}

void SearchDomain::remove(Int lb, Int ub) {
  assert(lb <= ub);
  if (std::holds_alternative<SetDomain>(_domain)) {
    return std::get<SetDomain>(_domain).remove(lb, ub);
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  if (lb <= lowerBound()) {
    return removeBelow(ub + 1);
  }
  if (upperBound() <= ub) {
    return removeAbove(lb - 1);
  }
  assert(lowerBound() < lb);
  assert(ub < upperBound());
  const Int left = lb - lowerBound();
  const Int newSize = static_cast<Int>(size()) - (ub - lb + 1);
  assert(newSize > 0);
  std::vector<Int> newDomain(newSize);
  std::iota(newDomain.begin(), newDomain.begin() + left, lowerBound());
  std::iota(newDomain.begin() + left, newDomain.end(), ub + 1);
  _domain = SetDomain(std::move(newDomain));
}

void SearchDomain::remove(const SortedUniqueVector& vals) {
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the value from the set domain:
    return std::get<SetDomain>(_domain).remove(vals);
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  remove(*vals);
}

void SearchDomain::remove(const IntervalDomain& vals) {
  remove(vals.lowerBound(), vals.upperBound());
}

void SearchDomain::remove(const SetDomain& other) {
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the value from the set domain:
    return std::get<SetDomain>(_domain).remove(other);
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  remove(other.values());
}

void SearchDomain::remove(const SearchDomain& other) {
  std::visit<void>([&](const auto& o) { return this->remove(o); },
                   other._domain);
}

void SearchDomain::removeAllValuesExcept(const std::vector<Int>& vals) {
  assert(std::ranges::adjacent_find(vals, std::greater_equal<>()) ==
         vals.end());
  assert(std::holds_alternative<IntervalDomain>(_domain));
  if (vals.empty()) {
    throw InconsistencyException("SearchDomain::intersect: Empty domain");
  }
  if (sortedVectorIsInterval(vals)) {
    std::get<IntervalDomain>(_domain).intersect((vals).front(), (vals).back());
    return;
  }
  Int begin = 0;
  Int end = static_cast<Int>(vals.size()) - 1;
  while (begin < end && vals[begin] < lowerBound()) {
    ++begin;
  }
  while (end >= 0 && upperBound() < vals[end]) {
    --end;
  }
  if (begin > end) {
    throw InconsistencyException("SearchDomain::intersect: Empty domain");
  }
  std::vector<Int> newDomain;
  newDomain.reserve(end - begin + 1);
  std::copy(vals.begin() + begin, vals.begin() + end + 1,
            std::back_inserter(newDomain));
  _domain = SetDomain(std::move(newDomain));
}

void SearchDomain::removeAllValuesExcept(const SortedUniqueVector& values) {
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the values from the set domain:
    return std::get<SetDomain>(_domain).intersect(values);
  }
  removeAllValuesExcept(*values);
}

void SearchDomain::removeAllValuesExcept(Int lb, Int ub) {
  removeBelow(lb);
  removeAbove(ub);
}

void SearchDomain::removeAllValuesExcept(const SetDomain& other) {
  if (std::holds_alternative<SetDomain>(_domain)) {
    return std::get<SetDomain>(_domain).intersect(other);
  }
  removeAllValuesExcept(other.values());
}

void SearchDomain::removeAllValuesExcept(const SearchDomain& other) {
  if (std::holds_alternative<SetDomain>(other._domain)) {
    removeAllValuesExcept(std::get<SetDomain>(other._domain));
    return;
  }
  assert(std::holds_alternative<IntervalDomain>(other._domain));
  removeAllValuesExcept(std::get<IntervalDomain>(other._domain).lowerBound(),
                        std::get<IntervalDomain>(other._domain).upperBound());
}

bool SearchDomain::isDisjoint(Int lb, Int ub) const {
  return std::visit<bool>(
      [&](const auto& dom) { return dom.isDisjoint(lb, ub); }, _domain);
}

bool SearchDomain::isDisjoint(const SortedUniqueVector& vals) const {
  return std::visit<bool>([&](const auto& dom) { return dom.isDisjoint(vals); },
                          _domain);
}

bool SearchDomain::isDisjoint(const IntervalDomain& other) const {
  return std::visit<bool>(
      [&](const auto& dom) { return dom.isDisjoint(other); }, _domain);
}

bool SearchDomain::isDisjoint(const SetDomain& other) const {
  return std::visit<bool>(
      [&](const auto& dom) { return dom.isDisjoint(other); }, _domain);
}

bool SearchDomain::isDisjoint(const SearchDomain& other) const {
  if (std::holds_alternative<IntervalDomain>(other._domain)) {
    return isDisjoint(std::get<IntervalDomain>(other._domain));
  }
  return isDisjoint(std::get<SetDomain>(other._domain));
}

void SearchDomain::fix(Int value) {
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the value from the set domain:
    std::get<SetDomain>(_domain).fix(value);
    return;
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  std::get<IntervalDomain>(_domain).fix(value);
}

bool SearchDomain::isEqual(Int lb, Int ub) const {
  return std::visit<bool>([&](const auto& dom) { return dom.isEqual(lb, ub); },
                          _domain);
}

bool SearchDomain::operator==(const SortedUniqueVector& vals) const {
  return std::visit<bool>([&](const auto& dom) { return dom == vals; },
                          _domain);
}

bool SearchDomain::operator==(const IntervalDomain& other) const {
  return std::visit<bool>([&](const auto& dom) { return dom == other; },
                          _domain);
}

bool SearchDomain::operator==(const SetDomain& other) const {
  return std::visit<bool>([&](const auto& dom) { return dom == other; },
                          _domain);
}

bool SearchDomain::operator==(const SearchDomain& other) const {
  return std::visit<bool>([&](const auto& o) { return *this == o; },
                          other._domain);
}

bool SearchDomain::operator!=(const SortedUniqueVector& vals) const {
  return std::visit<bool>([&](const auto& dom) { return dom != vals; },
                          _domain);
}

bool SearchDomain::operator!=(const IntervalDomain& other) const {
  return std::visit<bool>([&](const auto& dom) { return dom != other; },
                          _domain);
}

bool SearchDomain::operator!=(const SetDomain& other) const {
  return std::visit<bool>([&](const auto& dom) { return dom != other; },
                          _domain);
}

bool SearchDomain::operator!=(const SearchDomain& other) const {
  return std::visit<bool>([&](const auto& o) { return !(*this == o); },
                          other._domain);
}

}  // namespace atlantis
