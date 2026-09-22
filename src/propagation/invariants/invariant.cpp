#include "atlantis/propagation/invariants/invariant.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

Invariant::Invariant(SolverBase& solver, const Int nullState)
    : _solver(solver), _state(NULL_TIMESTAMP, nullState) {}

void Invariant::registerDefinedVar(const VarId id) {
  if (_primaryDefinedVar == NULL_ID) {
    _primaryDefinedVar = id;
  } else {
    _definedVars.push_back(id);
  }
  _solver.registerDefinedVar(id, _id);
}

void Invariant::updateValue(const Timestamp ts, const VarId id,
                            const Int val) const {
  _solver.updateValue(ts, id, val);
}

void Invariant::incValue(const Timestamp ts, const VarId id,
                         const Int val) const {
  _solver.incValue(ts, id, val);
}

void Invariant::setLevel(const size_t newLevel) noexcept { _level = newLevel; }

VarViewId Invariant::dynamicInputVar(Timestamp) const noexcept {
    return VAR_VIEW_NULL_ID}

InvariantId Invariant::id() const noexcept {
  return _id;
}

void Invariant::setId(const InvariantId id) { _id = id; }

VarId Invariant::primaryDefinedVar() const { return _primaryDefinedVar; }

const std::vector<VarId>& Invariant::nonPrimaryDefinedVars() const {
  return _definedVars;
}
}  // namespace atlantis::propagation
