#include "constraints/inSparseDomain.hpp"

#include "core/engine.hpp"

InSparseDomain::InSparseDomain(VarId violationId, VarId x,
                               const std::vector<DomainEntry>& domain)
    : Constraint(NULL_ID, violationId),
      _x(x),
      _offset(domain.front().lowerBound) {
  _modifiedVars.reserve(1);
  assert(domain.size() > 0);
  for (const auto& domEntry : domain) {
    assert(domEntry.lowerBound <= domEntry.upperBound);
  }
  for (size_t i = 1; i < domain.size(); ++i) {
    assert(domain[i - 1].upperBound < domain[i].lowerBound);
  }
  _valueViolation.resize(domain.back().upperBound - _offset + 1, 0);
  for (size_t i = 1; i < domain.size(); ++i) {
    for (Int val = domain[i - 1].upperBound + 1; val < domain[i].lowerBound;
         ++val) {
      _valueViolation[val - _offset] =
          std::min(val - domain[i - 1].upperBound, domain[i].lowerBound - val);
    }
  }
}

void InSparseDomain::init(Timestamp, Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void InSparseDomain::recompute(Timestamp ts, Engine& engine) {
  const Int value = engine.value(ts, _x);
  if (value < _offset) {
    updateValue(ts, engine, _violationId, _offset - value);
    return;
  }
  if (value >= _offset + static_cast<Int>(_valueViolation.size())) {
    updateValue(ts, engine, _violationId,
                value - _offset - _valueViolation.size() + 1);
    return;
  }
  updateValue(ts, engine, _violationId, _valueViolation[value - _offset]);
}

void InSparseDomain::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId InSparseDomain::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    default:
      return NULL_ID;
  }
}

void InSparseDomain::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void InSparseDomain::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
