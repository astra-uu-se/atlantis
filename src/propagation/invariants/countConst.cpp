#include "atlantis/propagation/invariants/countConst.hpp"

#include <utility>
#include <vector>

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

CountConst::CountConst(SolverBase& solver, const VarId output, const Int needle,
                       std::vector<VarViewId>&& vars, const Int outputOffset)
    : Invariant(solver),
      _outputOffset(outputOffset),
      _output(output),
      _needle(needle),
      _vars(std::move(vars)) {}

CountConst::CountConst(SolverBase& solver, const VarViewId output,
                       const Int needle, std::vector<VarViewId>&& vars,
                       const Int outputOffset)
    : CountConst(solver, VarId(output), needle, std::move(vars), outputOffset) {
  assert(output.isVar());
}

void CountConst::registerVars() {
  // precondition: this invariant must be registered with the solver before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  registerDefinedVar(_output);
}

void CountConst::updateBounds(const bool widenOnly) {
  _solver.updateBounds(
      _output, 0, static_cast<Int>(_vars.size()) + _outputOffset, widenOnly);
}

void CountConst::recompute(const Timestamp ts) {
  Int count = _outputOffset;
  for (const auto& var : _vars) {
    count += static_cast<Int>(_solver.value(ts, var) == _needle);
  }
  updateValue(ts, _output, count);
}

void CountConst::notifyInputChanged(const Timestamp ts, const LocalId id) {
  assert(id < _vars.size());
  const Int newValue = _solver.value(ts, _vars[id]) == _needle ? 1 : 0;
  const Int committedValue =
      _solver.committedValue(_vars[id]) == _needle ? 1 : 0;
  if (newValue == committedValue) {
    return;
  }
  incValue(ts, _output, newValue - committedValue);
}

VarViewId CountConst::nextInput(const Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _vars.size()) {
    return _vars[index];
  }
  return NULL_ID;  // Done
}

void CountConst::notifyCurrentInputChanged(const Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

}  // namespace atlantis::propagation
