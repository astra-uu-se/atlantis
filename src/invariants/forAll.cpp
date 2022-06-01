#include "invariants/forAll.hpp"

ForAll::ForAll(VarId output, std::vector<VarId> varArray)
    : Invariant(),
      _output(output),
      _varArray(std::move(varArray)),
      _localPriority(_varArray.size()) {
  _modifiedVars.reserve(_varArray.size());
}

void ForAll::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _varArray.size(); ++i) {
    engine.registerInvariantInput(_id, _varArray[i], i);
  }
  registerDefinedVariable(engine, _output);
}

void ForAll::updateBounds(Engine& engine, bool widenOnly) {
  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();
  for (const VarId input : _varArray) {
    lb = std::max(lb, engine.lowerBound(input));
    ub = std::max(ub, engine.upperBound(input));
  }
  engine.updateBounds(_output, std::max(Int(0), lb), ub, widenOnly);
}

void ForAll::recompute(Timestamp ts, Engine& engine) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, engine.value(ts, _varArray[i]));
  }
  assert(_localPriority.minPriority(ts) >= 0);
  updateValue(ts, engine, _output, _localPriority.maxPriority(ts));
}

void ForAll::notifyInputChanged(Timestamp ts, Engine& engine, LocalId id) {
  _localPriority.updatePriority(
      ts, id, std::max(Int(0), engine.value(ts, _varArray[id])));
  assert(_localPriority.minPriority(ts) >= 0);
  updateValue(ts, engine, _output, _localPriority.maxPriority(ts));
}

VarId ForAll::nextInput(Timestamp ts, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void ForAll::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  notifyInputChanged(ts, engine, _state.value(ts));
}

void ForAll::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  _localPriority.commitIf(ts);
}