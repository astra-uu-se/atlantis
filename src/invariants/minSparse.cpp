#include "invariants/minSparse.hpp"

#include <utility>
MinSparse::MinSparse(VarId output, std::vector<VarId> varArray)
    : Invariant(),
      _output(output),
      _varArray(std::move(varArray)),
      _localPriority(_varArray.size()) {
  assert(!_varArray.empty());
  _modifiedVars.reserve(_varArray.size());
}

void MinSparse::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _varArray.size(); ++i) {
    engine.registerInvariantInput(_id, _varArray[i], i);
  }
  registerDefinedVariable(engine, _output);
}

void MinSparse::updateBounds(Engine& engine, bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::max();
  for (const VarId input : _varArray) {
    lb = std::min(lb, engine.lowerBound(input));
    ub = std::min(ub, engine.upperBound(input));
  }
  engine.updateBounds(_output, lb, ub, widenOnly);
}

void MinSparse::recompute(Timestamp ts, Engine& engine) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, engine.value(ts, _varArray[i]));
  }
  updateValue(ts, engine, _output, _localPriority.minPriority(ts));
}

void MinSparse::notifyInputChanged(Timestamp ts, Engine& engine, LocalId id) {
  _localPriority.updatePriority(ts, id, engine.value(ts, _varArray[id]));
  updateValue(ts, engine, _output, _localPriority.minPriority(ts));
}

VarId MinSparse::nextInput(Timestamp ts, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void MinSparse::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  notifyInputChanged(ts, engine, _state.value(ts));
}

void MinSparse::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  _localPriority.commitIf(ts);
}