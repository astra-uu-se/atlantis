#include "atlantis/propagation/invariants/countConst.hpp"

#include <utility>
#include <vector>

namespace atlantis::propagation {

CountConst::CountConst(SolverBase& solver, VarId output, Int y,
                       std::vector<VarId>&& vars)
    : Invariant(solver), _output(output), _y(y), _vars(std::move(vars)) {}

void CountConst::registerVars() {
  // precondition: this invariant must be registered with the solver before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  registerDefinedVar(_output);
}

void CountConst::updateBounds(bool widenOnly) {
  _solver.updateBounds(_output, 0, static_cast<Int>(_vars.size()), widenOnly);
}

void CountConst::recompute(Timestamp ts) {
  Int count = 0;
  for (const auto& var : _vars) {
    count += static_cast<Int>(_solver.value(ts, var) == _y);
  }
  updateValue(ts, _output, count);
}

void CountConst::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _vars.size());
  const Int newValue = static_cast<Int>(_solver.value(ts, _vars[id]) == _y);
  const Int committedValue =
      static_cast<Int>(_solver.committedValue(_vars[id]) == _y);
  if (newValue == committedValue) {
    return;
  }
  incValue(ts, _output, newValue - committedValue);
}

VarId CountConst::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _vars.size()) {
    return _vars[index];
  }
  return NULL_ID;  // Done
}

void CountConst::notifyCurrentInputChanged(Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

}  // namespace atlantis::propagation
