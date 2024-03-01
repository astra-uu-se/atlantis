#include "atlantis/propagation/views/inDomain.hpp"

#include <cassert>
#include <limits>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/variables/committable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

inline bool all_in_range(size_t start, size_t stop,
                         std::function<bool(size_t)>&& predicate) {
  std::vector<size_t> vec(stop - start);
  for (size_t i = 0; i < stop - start; ++i) {
    vec.at(i) = start + i;
  }
  return std::all_of(vec.begin(), vec.end(), std::move(predicate));
}

InDomain::InDomain(SolverBase& solver, VarId parentId,
                   std::vector<DomainEntry>&& domain)
    : IntView(solver, parentId),
      _domain(std::move(domain)),
      _cache(NULL_TIMESTAMP, std::pair<Int, Int>(0, compute(0))) {
  assert(!_domain.empty());
  assert(std::all_of(_domain.begin(), _domain.end(), [&](const auto& domEntry) {
    return domEntry.lowerBound <= domEntry.upperBound;
  }));
  assert(all_in_range(1u, _domain.size(), [&](const size_t i) {
    return _domain.at(i - 1).upperBound < _domain.at(i).lowerBound;
  }));
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
  const Int val = _solver.value(ts, _parentId);
  if (_cache.get(ts).first != val) {
    _cache.set(ts, std::pair<Int, Int>{val, compute(val)});
  }
  return _cache.get(ts).second;
}

Int InDomain::committedValue() {
  const Int val = _solver.committedValue(_parentId);
  if (_cache.get(_cache.tmpTimestamp()).first != val) {
    _cache.commitValue(std::pair<Int, Int>{val, compute(val)});
  }
  return _cache.get(_cache.tmpTimestamp()).second;
}

Int InDomain::lowerBound() const {
  const Int parentLb = _solver.lowerBound(_parentId);
  const Int parentUb = _solver.upperBound(_parentId);
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
  const Int parentLb = _solver.lowerBound(_parentId);
  const Int parentUb = _solver.upperBound(_parentId);
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

}  // namespace atlantis::propagation
