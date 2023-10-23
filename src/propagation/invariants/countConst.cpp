#include "propagation/invariants/countConst.hpp"

namespace atlantis::propagation {

CountConst::CountConst(SolverBase& solver, VarId output, Int y,
                       std::vector<VarId> variables)
    : Invariant(solver),
      _output(output),
      _y(y),
      _variables(std::move(variables)),
      _hasCountValue(_variables.size(), 0) {
  _modifiedVars.reserve(_variables.size());
}

void CountConst::registerVars() {
  // precondition: this invariant must be registered with the solver before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _variables.size(); ++i) {
    _solver.registerInvariantInput(_id, _variables[i], i);
  }
  registerDefinedVariable(_output);
}

void CountConst::updateBounds(bool widenOnly) {
  _solver.updateBounds(_output, 0, _variables.size(), widenOnly);
}

void CountConst::recompute(Timestamp ts) {
  Int count = 0;
  for (size_t i = 0; i < _variables.size(); ++i) {
    count += static_cast<Int>(_solver.value(ts, _variables[i]) == _y);
  }
  updateValue(ts, _output, count);
}

void CountConst::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _hasCountValue.size());
  const Int newValue =
      static_cast<Int>(_solver.value(ts, _variables[id]) == _y);
  if (_hasCountValue[id] == newValue) {
    return;
  }
  incValue(ts, _output, newValue - _hasCountValue[id]);
}

VarId CountConst::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _variables.size()) {
    return _variables[index];
  }
  return NULL_ID;  // Done
}

void CountConst::notifyCurrentInputChanged(Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

void CountConst::commit(Timestamp ts) {
  Invariant::commit(ts);
  for (size_t i = 0; i < _hasCountValue.size(); ++i) {
    _hasCountValue[i] =
        static_cast<Int>(_solver.committedValue(_variables[i]) == _y);
  }
}
}  // namespace atlantis::propagation