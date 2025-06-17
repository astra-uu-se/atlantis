#include <algorithm>
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

bool IntervalDomain::isDisjoint(const SetDomain& other) const {
  return other.upperBound() < _lb || _ub < other.lowerBound();
}

bool IntervalDomain::isDisjoint(const IntervalDomain& other) const {
  return other.upperBound() < _lb || _ub < other.lowerBound();
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

bool IntervalDomain::operator==(const IntervalDomain& other) const {
  return other._lb == _lb && other._ub == _ub;
}

bool IntervalDomain::operator!=(const IntervalDomain& other) const {
  return !(*this == other);
}

SetDomain::SetDomain(std::vector<Int>&& values) : _values(std::move(values)) {
  if (_values.empty()) {
    throw InconsistencyException("SetDomain::SetDomain: empty domain");
  }
  std::ranges::sort(_values.begin(), _values.end());
  _values.erase(std::ranges::unique(_values).begin(), _values.end());
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
  return std::ranges::binary_search(_values.begin(), _values.end(), value);
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
}

void SetDomain::removeBelow(Int newLowerBound) {
  if (newLowerBound <= lowerBound()) {
    return;
  }
  if (upperBound() < newLowerBound) {
    throw InconsistencyException("SetDomain::removeBelow: Empty domain");
  }
  Int offset = 0;
  for (size_t i = 0; i < _values.size() && _values[i] < newLowerBound; ++i) {
    ++offset;
  }
  _values.erase(_values.begin(), _values.begin() + offset);
}

void SetDomain::removeAbove(Int newUpperBound) {
  if (upperBound() <= newUpperBound) {
    return;
  }
  if (newUpperBound < lowerBound()) {
    throw InconsistencyException("SetDomain::removeAbove: Empty domain");
  }
  Int offset = static_cast<Int>(_values.size()) - 1;
  for (Int i = static_cast<Int>(_values.size()) - 1;
       0 <= i && newUpperBound < _values[i]; --i) {
    --offset;
  }
  _values.erase(_values.begin() + offset, _values.end());
}

void SetDomain::remove(const std::vector<Int>& values) {
  std::vector<Int> cpy(values);
  std::ranges::sort(cpy.begin(), cpy.end());

  size_t i = 0;
  Int j = 0;
  while (i < cpy.size() && j < static_cast<Int>(_values.size())) {
    if (cpy[i] > _values[j]) {
      ++j;
    } else {
      if (cpy[i] == _values[j]) {
        _values.erase(_values.begin() + j);
      }
      ++i;
    }
  }
  if (_values.empty()) {
    throw InconsistencyException("SetDomain::remove: Empty domain");
  }
}

bool SetDomain::isDisjoint(const IntervalDomain& other) const {
  return other.upperBound() < lowerBound() || upperBound() < other.lowerBound();
}

bool SetDomain::isDisjoint(const SetDomain& other) const {
  size_t i = 0;
  size_t j = 0;
  while (i < _values.size() && j < other._values.size()) {
    if (_values[i] < other._values[j]) {
      ++i;
    } else if (_values[i] > other._values[j]) {
      ++j;
    } else {
      return false;
    }
  }
  return true;
}

void SetDomain::intersect(const std::vector<Int>& otherVals) {
  std::vector<Int> cpy(otherVals);
  std::ranges::sort(cpy.begin(), cpy.end());
  cpy.erase(std::ranges::unique(cpy).begin(), cpy.end());

  std::vector<Int> newValues;
  newValues.reserve(std::min(_values.size(), otherVals.size()));

  std::ranges::set_intersection(_values, cpy, std::back_inserter(newValues));

  _values = std::move(newValues);
  if (_values.empty()) {
    throw InconsistencyException("SetDomain::intersect: Empty domain");
  }
}

void SetDomain::fix(Int value) {
  if (!contains(value)) {
    throw InconsistencyException("SetDomain::fix: Empty domain");
  }
  _values = std::vector<Int>{value};
}

bool SetDomain::operator==(const SetDomain& other) const {
  return _values == other._values;
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

void SearchDomain::remove(const std::vector<Int>& values) {
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the value from the set domain:
    return std::get<SetDomain>(_domain).remove(values);
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  std::vector<Int> cpy(values);
  std::ranges::sort(cpy.begin(), cpy.end());
  for (const Int value : cpy) {
    remove(value);
  }
}

void SearchDomain::intersect(const std::vector<Int>& values) {
  if (std::holds_alternative<SetDomain>(_domain)) {
    // Remove the values from the set domain:
    return std::get<SetDomain>(_domain).intersect(values);
  }
  assert(std::holds_alternative<IntervalDomain>(_domain));
  std::vector<Int> cpy(values);
  std::ranges::sort(cpy.begin(), cpy.end());
  cpy.erase(std::ranges::unique(cpy).begin(), cpy.end());
  if (sortedVectorIsInterval(cpy)) {
    std::get<IntervalDomain>(_domain).intersect(cpy.front(), cpy.back());
    return;
  }
  Int begin = 0;
  Int end = static_cast<Int>(cpy.size()) - 1;
  while (begin < end && cpy[begin] < lowerBound()) {
    ++begin;
  }
  while (end >= 0 && upperBound() < cpy[end]) {
    --end;
  }
  if (begin > end) {
    throw InconsistencyException("SearchDomain::intersect: Empty domain");
  }
  std::vector<Int> newDomain;
  newDomain.reserve(end - begin + 1);
  std::copy(cpy.begin() + begin, cpy.begin() + end + 1,
            std::back_inserter(newDomain));
  _domain = SetDomain(std::move(newDomain));
}

void SearchDomain::intersect(Int lb, Int ub) {
  removeBelow(lb);
  removeAbove(ub);
}

void SearchDomain::intersect(const SearchDomain& other) {
  if (std::holds_alternative<SetDomain>(other._domain)) {
    intersect(std::get<SetDomain>(other._domain).values());
    return;
  }
  assert(std::holds_alternative<IntervalDomain>(other._domain));
  intersect(std::get<IntervalDomain>(other._domain).lowerBound(),
            std::get<IntervalDomain>(other._domain).upperBound());
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

bool SearchDomain::operator==(const SearchDomain& other) const {
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

bool SearchDomain::operator!=(const SearchDomain& other) const {
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

}  // namespace atlantis
