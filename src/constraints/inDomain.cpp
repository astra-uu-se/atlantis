#include "constraints/inDomain.hpp"

#include "core/engine.hpp"

InDomain::InDomain(VarId violationId, VarId x, std::vector<DomainEntry> domain)
    : Constraint(NULL_ID, violationId), _domain(domain), _x(x) {
  _modifiedVars.reserve(1);
  assert(_domain.size() > 1);
  for (const auto& domEntry : _domain) {
    assert(domEntry.lowerBound <= domEntry.upperBound);
  }
  for (size_t i = 1; i < _domain.size(); ++i) {
    assert(_domain[i - 1].upperBound < _domain[i].lowerBound);
  }
}

void InDomain::init(Timestamp, Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void InDomain::recompute(Timestamp ts, Engine& engine) {
  const Int value = engine.getValue(ts, _x);
  for (size_t i = 0; i < _domain.size(); ++i) {
    if (value > _domain[i].upperBound) {
      continue;
    }
    if (_domain[i].lowerBound <= value) {
      updateValue(ts, engine, _violationId, 0);
    } else if (i > 0) {
      updateValue(ts, engine, _violationId,
                  std::min(value - _domain[i - 1].upperBound,
                           _domain[i].lowerBound - value));
    } else {
      updateValue(ts, engine, _violationId, value - _domain[i].lowerBound);
    }
    return;
  }
}

void InDomain::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId InDomain::getNextInput(Timestamp ts, Engine&) {
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
