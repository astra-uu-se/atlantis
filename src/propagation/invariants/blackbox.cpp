#include "atlantis/propagation/invariants/blackbox.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>

#include "atlantis/propagation/variables/committableInt.hpp"

namespace atlantis::propagation {

Blackbox::Blackbox(SolverBase& solver,
                   std::shared_ptr<blackbox::BlackBoxFn> blackBoxFn,
                   std::vector<VarId>&& outputs,
                   std::vector<VarViewId>&& inputs)
    : Invariant(solver),
      _blackBoxFn(std::move(blackBoxFn)),
      _outputs(std::move(outputs)),
      _inputs(std::move(inputs)),
      _intIn(_inputs.size()),
      _intOut(_outputs.size()) {}

void Blackbox::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _inputs.size(); ++i) {
    _solver.registerInvariantInput(_id, _inputs[i], LocalId(i), false);
  }
  for (const VarId& output : _outputs) {
    registerDefinedVar(output);
  }
}

void Blackbox::recompute(Timestamp timestamp) {
  for (size_t i = 0; i < _inputs.size(); ++i) {
    _intIn[i] = _solver.value(timestamp, _inputs[i]);
  }

  _blackBoxFn->run(_intIn, _floatIn, _intOut, _floatOut);

  // Note: the outputs are deliberately not checked against their declared
  // domains. Unlike a CP propagator, a CBLS invariant legitimately takes
  // values that violate the constraints on its outputs -- that violation is
  // exactly what drives the search.
  for (size_t i = 0; i < _outputs.size(); ++i) {
    updateValue(timestamp, _outputs[i], _intOut[i]);
  }
}

void Blackbox::notifyInputChanged(Timestamp timestamp, LocalId localId) {
  assert(localId < _inputs.size());
  const Int newValue = _solver.value(timestamp, _inputs[localId]);
  const Int committedValue = _solver.committedValue(_inputs[localId]);
  if (newValue == committedValue) {
    return;
  }
  recompute(timestamp);
}

VarViewId Blackbox::nextInput(Timestamp timestamp) {
  const auto index = static_cast<size_t>(_state.incValue(timestamp, 1));
  assert(0 <= _state.value(timestamp));
  if (index < _inputs.size()) {
    return _inputs[index];
  }
  return VarViewId{NULL_ID};
}

void Blackbox::notifyCurrentInputChanged(Timestamp timestamp) {
  assert(static_cast<size_t>(_state.value(timestamp)) < _inputs.size());
  notifyInputChanged(timestamp, _state.value(timestamp));
}

}  // namespace atlantis::propagation
