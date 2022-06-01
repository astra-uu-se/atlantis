#include "views/inDomain.hpp"

#include "core/engine.hpp"

InDomain::InDomain(VarId parentId, std::vector<DomainEntry>&& domain)
    : IntView(parentId),
      _domain(std::move(domain)),
      _cache(NULL_TIMESTAMP, std::pair<Int, Int>(0, compute(0))) {
#ifndef NDEBUG
  assert(_domain.size() >= 1);
  for (const auto& domEntry : _domain) {
    assert(domEntry.lowerBound <= domEntry.upperBound);
  }
  for (size_t i = 1; i < _domain.size(); ++i) {
    assert(_domain[i - 1].upperBound < _domain[i].lowerBound);
  }
#endif
}

Int InDomain::compute(const Int val) const {
  if (val < _domain.front().lowerBound) {
    return _domain.front().lowerBound - val;
  }
  for (size_t i = 0; i < _domain.size(); ++i) {
    if (val > _domain[i].upperBound) {
      continue;
    }
    if (val < _domain[i].lowerBound) {
      assert(i > 0);
      assert(val > _domain[i - 1].upperBound);
      return std::min(val - _domain[i - 1].upperBound,
                      _domain[i].lowerBound - val);
    } else {
      assert(_domain[i].lowerBound <= val && val <= _domain[i].upperBound);
      return 0;
    }
  }
  assert(_domain.back().upperBound < val);
  return val - _domain.back().upperBound;
}

Int InDomain::value(Timestamp ts) {
  const Int val = _engine->value(ts, _parentId);
  if (_cache.get(ts).first != val) {
    _cache.set(ts, std::pair<Int, Int>{val, compute(val)});
  }
  return _cache.get(ts).second;
}

Int InDomain::committedValue() {
  const Int val = _engine->committedValue(_parentId);
  if (_cache.get(_cache.tmpTimestamp()).first != val) {
    _cache.commitValue(std::pair<Int, Int>{val, compute(val)});
  }
  return _cache.get(_cache.tmpTimestamp()).second;
}

Int InDomain::lowerBound() const {
  const Int parentLb = _engine->lowerBound(_parentId);
  const Int parentUb = _engine->upperBound(_parentId);
  Int minViol = std::numeric_limits<Int>::max();
  for (const auto& [dLb, dUb] : _domain) {
    if (parentUb < dLb) {
      return std::min(minViol, dLb - parentUb);
    } else if (parentLb <= dUb) {
      return 0;
    } else {
      // parentLb > dUb
      minViol = std::min(minViol, parentLb - dUb);
    }
  }
  return minViol;
}

Int InDomain::upperBound() const {
  const Int parentLb = _engine->lowerBound(_parentId);
  const Int parentUb = _engine->upperBound(_parentId);
  Int maxViol = std::numeric_limits<Int>::max();
  for (const auto& [dLb, dUb] : _domain) {
    if (parentUb < dLb) {
      return std::min(maxViol, dLb - parentLb);
    } else if (parentLb <= dUb) {
      if (parentLb < dLb) {
        maxViol = std::min(maxViol, dLb - parentLb);
      } else if (dUb < parentUb) {
        maxViol = std::min(maxViol, parentUb - dUb);
      } else {
        return 0;
      }
    } else {
      // parentLb > dUb
      maxViol = std::min(maxViol, parentUb - dUb);
    }
  }
  return maxViol;
}