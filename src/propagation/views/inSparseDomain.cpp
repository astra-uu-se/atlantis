#include "atlantis/propagation/views/inSparseDomain.hpp"

#include <cassert>
#include <limits>

namespace atlantis::propagation {

inline bool all_in_range(size_t start, size_t stop,
                         std::function<bool(size_t)>&& predicate) {
  std::vector<size_t> vec(stop - start);
  for (size_t i = 0; i < stop - start; ++i) {
    vec.at(i) = start + i;
  }
  return std::all_of(vec.begin(), vec.end(), std::move(predicate));
}

InSparseDomain::InSparseDomain(SolverBase& solver, VarViewId parentId,
                               const std::vector<DomainEntry>& domain)
    : IntView(solver, parentId), _offset(domain.front().lowerBound) {
  assert(!domain.empty());
  assert(std::all_of(domain.begin(), domain.end(), [&](const auto& domEntry) {
    return domEntry.lowerBound <= domEntry.upperBound;
  }));
  assert(all_in_range(1u, domain.size(), [&](const size_t i) {
    return domain.at(i - 1).upperBound < domain.at(i).lowerBound;
  }));
  _valueViolation.resize(domain.back().upperBound - _offset + 1, 0);
  for (size_t i = 1; i < domain.size(); ++i) {
    for (Int val = domain[i - 1].upperBound + 1; val < domain[i].lowerBound;
         ++val) {
      _valueViolation[val - _offset] =
          std::min(val - domain[i - 1].upperBound, domain[i].lowerBound - val);
    }
  }
}

Int InSparseDomain::value(Timestamp ts) {
  const Int val = _solver.value(ts, _parentId);
  if (val < _offset) {
    return _offset - val;
  }
  if (val >= _offset + static_cast<Int>(_valueViolation.size())) {
    return val - _offset - static_cast<Int>(_valueViolation.size()) + 1;
  }
  return _valueViolation[val - _offset];
}

Int InSparseDomain::committedValue() {
  const Int val = _solver.committedValue(_parentId);
  if (val < _offset) {
    return _offset - val;
  }
  if (val >= _offset + static_cast<Int>(_valueViolation.size())) {
    return val - _offset - static_cast<Int>(_valueViolation.size()) + 1;
  }
  return _valueViolation[val - _offset];
}

Int InSparseDomain::lowerBound() const {
  const Int parentLb = _solver.lowerBound(_parentId);
  const Int parentUb = _solver.upperBound(_parentId);
  const Int dLb = _offset;
  const Int dUb = _offset + static_cast<Int>(_valueViolation.size()) - 1;

  if (parentUb < _offset) {
    return dLb - parentUb;
  } else if (dUb < parentLb) {
    return parentLb - dUb;
  }
  const Int begin = std::max<Int>(0, parentLb - _offset);
  const Int end = std::min<Int>(static_cast<Int>(_valueViolation.size()),
                                parentUb - _offset + 1);

  return *std::min_element(_valueViolation.begin() + begin,
                           _valueViolation.begin() + end);
}

Int InSparseDomain::upperBound() const {
  const Int parentLb = _solver.lowerBound(_parentId);
  const Int parentUb = _solver.upperBound(_parentId);
  const Int dLb = _offset;
  const Int dUb = _offset + static_cast<Int>(_valueViolation.size()) - 1;

  if (parentUb < dLb) {
    return dLb - parentLb;
  } else if (dUb < parentLb) {
    return parentUb - dUb;
  }
  const Int begin = std::max<Int>(0, parentLb - _offset);
  const Int end = std::min<Int>(static_cast<Int>(_valueViolation.size()),
                                parentUb - _offset + 1);

  return *std::max_element(_valueViolation.begin() + begin,
                           _valueViolation.begin() + end);
}

}  // namespace atlantis::propagation
