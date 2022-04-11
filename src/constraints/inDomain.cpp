#include "constraints/inDomain.hpp"

#include "core/engine.hpp"

InDomain::InDomain(VarId violationId, VarId x, std::vector<DomainEntry> domain)
    : Constraint(NULL_ID, violationId), _domain(domain), _x(x) {
  _modifiedVars.reserve(1);
  assert(_domain.size() >= 1);
  for (const auto& domEntry : _domain) {
    assert(domEntry.lowerBound <= domEntry.upperBound);
  }
  for (size_t i = 1; i < _domain.size(); ++i) {
    assert(_domain[i - 1].upperBound < _domain[i].lowerBound);
  }
}

void InDomain::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void InDomain::updateBounds(Engine& engine) {
  const Int xLb = engine.lowerBound(_x);
  const Int xUb = engine.upperBound(_x);
  Int minViol = std::numeric_limits<Int>::max();
  Int maxViol = std::numeric_limits<Int>::max();
  for (const auto& [dLb, dUb] : _domain) {
    if (xUb < dLb) {
      minViol = std::min(minViol, dLb - xUb);
      maxViol = std::min(maxViol, dLb - xLb);
      break;
    } else if (xLb <= dUb) {
      minViol = 0;
      if (xLb < dLb) {
        maxViol = std::min(maxViol, dLb - xLb);
      } else if (dUb < xUb) {
        maxViol = std::min(maxViol, xUb - dUb);
      } else {
        maxViol = 0;
      }
    } else {
      // xLb > dUb
      minViol = std::min(minViol, xLb - dUb);
      maxViol = std::min(maxViol, xUb - dUb);
    }
  }
  engine.updateBounds(_violationId, minViol, maxViol);
}

void InDomain::recompute(Timestamp ts, Engine& engine) {
  const Int value = engine.value(ts, _x);
  if (value < _domain.front().lowerBound) {
    updateValue(ts, engine, _violationId, _domain.front().lowerBound - value);
    return;
  }
  for (size_t i = 0; i < _domain.size(); ++i) {
    if (value > _domain[i].upperBound) {
      continue;
    }
    if (value < _domain[i].lowerBound) {
      assert(i > 0);
      assert(value > _domain[i - 1].upperBound);
      updateValue(ts, engine, _violationId,
                  std::min(value - _domain[i - 1].upperBound,
                           _domain[i].lowerBound - value));
    } else {
      assert(_domain[i].lowerBound <= value && value <= _domain[i].upperBound);
      updateValue(ts, engine, _violationId, 0);
    }
    return;
  }
  assert(_domain.back().upperBound < value);
  updateValue(ts, engine, _violationId, value - _domain.back().upperBound);
}

void InDomain::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId InDomain::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    default:
      return NULL_ID;
  }
}

void InDomain::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void InDomain::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
