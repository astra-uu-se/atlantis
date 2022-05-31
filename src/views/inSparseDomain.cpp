#include "views/inSparseDomain.hpp"

#include "core/engine.hpp"

InSparseDomain::InSparseDomain(VarId parentId,
                               const std::vector<DomainEntry>& domain)
    : IntView(parentId), _offset(domain.front().lowerBound) {
#ifndef NDEBUG
  assert(domain.size() > 0);
  for (const auto& domEntry : domain) {
    assert(domEntry.lowerBound <= domEntry.upperBound);
  }
  for (size_t i = 1; i < domain.size(); ++i) {
    assert(domain[i - 1].upperBound < domain[i].lowerBound);
  }
#endif
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
  const Int val = _engine->value(ts, _parentId);
  if (val < _offset) {
    return _offset - val;
  }
  if (val >= _offset + static_cast<Int>(_valueViolation.size())) {
    return val - _offset - _valueViolation.size() + 1;
  }
  return _valueViolation[val - _offset];
}

Int InSparseDomain::committedValue() {
  const Int val = _engine->committedValue(_parentId);
  if (val < _offset) {
    return _offset - val;
  }
  if (val >= _offset + static_cast<Int>(_valueViolation.size())) {
    return val - _offset - _valueViolation.size() + 1;
  }
  return _valueViolation[val - _offset];
}

Int InSparseDomain::lowerBound() const {
  const Int parentLb = _engine->lowerBound(_parentId);
  const Int parentUb = _engine->upperBound(_parentId);
  const Int dLb = _offset;
  const Int dUb = _offset + _valueViolation.size() - 1;

  if (parentUb < _offset) {
    return dLb - parentUb;
  } else if (dUb < parentLb) {
    return parentLb - dUb;
  }
  const Int indexBegin = std::max<Int>(0, parentLb - _offset);
  const Int indexEnd = std::min<Int>(
      static_cast<Int>(_valueViolation.size()) - 1, parentUb - _offset);

  Int minViol = std::numeric_limits<Int>::max();
  for (Int i = indexBegin; i <= indexEnd; ++i) {
    minViol = std::min<Int>(minViol, _valueViolation[i]);
  }
  return minViol;
}

Int InSparseDomain::upperBound() const {
  const Int parentLb = _engine->lowerBound(_parentId);
  const Int parentUb = _engine->upperBound(_parentId);
  const Int dLb = _offset;
  const Int dUb = _offset + _valueViolation.size() - 1;

  if (parentUb < dLb) {
    return dLb - parentLb;
  } else if (dUb < parentLb) {
    return parentUb - dUb;
  }
  const Int indexBegin = std::max<Int>(0, parentLb - _offset);
  const Int indexEnd = std::min<Int>(
      static_cast<Int>(_valueViolation.size()) - 1, parentUb - _offset);

  Int maxViol = std::numeric_limits<Int>::min();
  for (Int i = indexBegin; i <= indexEnd; ++i) {
    maxViol = std::max<Int>(maxViol, _valueViolation[i]);
  }
  return maxViol;
}