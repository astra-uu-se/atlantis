#include "atlantis/propagation/arcs.hpp"

#include "atlantis/exceptions/exceptions.hpp"

namespace atlantis::propagation {

namespace invariant {

IncomingArcContainer::IncomingArcContainer()
    : _incomingStatic(), _incomingDynamic() {}

size_t IncomingArcContainer::numArcs() const {
  return _incomingStatic.size() + _incomingDynamic.size();
}

const std::vector<VarIdBase>& IncomingArcContainer::incomingStatic() const {
  return _incomingStatic;
}

const std::vector<IncomingDynamicArc>& IncomingArcContainer::incomingDynamic()
    const {
  return _incomingDynamic;
}

LocalId IncomingArcContainer::emplaceStatic(VarIdBase varId) {
  _incomingStatic.emplace_back(varId);
  return static_cast<LocalId>(numArcs() - 1);
}

LocalId IncomingArcContainer::emplaceDynamic(VarIdBase varId,
                                             size_t localInvariantIndex) {
  if (!_incomingStatic.empty()) {
    // All dynamic input variables must be registered before registering
    // static input variables.
    throw OutOfOrderIndexRegistration();
  }
  _incomingDynamic.emplace_back(varId, localInvariantIndex);
  return static_cast<LocalId>(_incomingDynamic.size() - 1);
}

}  // namespace invariant

namespace var {
OutgoingArc::OutgoingArc(const LocalId localId, const InvariantId invariantId)
    : _localId(localId), _invariantId(invariantId) {}

LocalId OutgoingArc::localId() const { return _localId; }
InvariantId OutgoingArc::invariantId() const { return _invariantId; }

void OutgoingArc::setInvariantNullId() { _invariantId = NULL_ID; }

void OutgoingDynamicArcContainer::swap(Timestamp ts, size_t index) {
  // sanity:
  assert(index < _metaIndices.size());
  assert(0 <= _numActiveArcs.value(ts));
  assert(static_cast<size_t>(_numActiveArcs.value(ts)) < _indices.size());
  // get meta index:
  const Int metaIndex = _metaIndices[index].value(ts);
  assert(0 <= metaIndex);
  assert(static_cast<size_t>(metaIndex) < _indices.size());
  // get the last active index and meta index:
  const size_t index2 = static_cast<size_t>(_numActiveArcs.value(ts));
  assert(index2 < _indices.size());
  const Int metaIndex2 = static_cast<size_t>(_metaIndices[index2].value(ts));
  assert(0 <= metaIndex2);
  assert(static_cast<size_t>(metaIndex2) < _indices.size());
  // perform index swap:
  _indices[metaIndex].setValue(ts, index2);
  _indices[metaIndex2].setValue(ts, index);
  // perform meta index swap:
  _metaIndices[index].setValue(ts, metaIndex2);
  _metaIndices[index2].setValue(ts, metaIndex);
}

OutgoingDynamicArcContainer::OutgoingDynamicArcContainer()
    : _arcs(), _indices(), _metaIndices(), _numActiveArcs(NULL_TIMESTAMP, 0) {}

const std::vector<CommittableInt>& OutgoingDynamicArcContainer::indices()
    const {
  return _indices;
}

const std::vector<OutgoingArc> OutgoingDynamicArcContainer::arcs() const {
  return _arcs;
}

size_t OutgoingDynamicArcContainer::size() const { return _arcs.size(); }

bool OutgoingDynamicArcContainer::empty() const { return _arcs.empty(); }

size_t OutgoingDynamicArcContainer::numActive(Timestamp ts) const {
  return _numActiveArcs.value(ts);
}

OutgoingArc& OutgoingDynamicArcContainer::at(size_t index) {
  return _arcs.at(index);
}

OutgoingArc& OutgoingDynamicArcContainer::operator[](size_t index) {
  assert(index < _arcs.size());
  return _arcs[index];
}

void OutgoingDynamicArcContainer::emplaceBack(LocalId localId,
                                              InvariantId invariantId) {
  _arcs.emplace_back(localId, invariantId);
  _indices.push_back(
      CommittableInt(NULL_TIMESTAMP, static_cast<Int>(_arcs.size()) - 1));
  _metaIndices.push_back(
      CommittableInt(NULL_TIMESTAMP, static_cast<Int>(_arcs.size()) - 1));
}

bool OutgoingDynamicArcContainer::isActive(Timestamp ts, size_t index) const {
  assert(index < _metaIndices.size());
  return static_cast<Int>(index) < _numActiveArcs.value(ts);
}

void OutgoingDynamicArcContainer::makeActive(Timestamp ts, size_t index) {
  assert(index < _metaIndices.size());
  const Int metaIndex = static_cast<size_t>(_metaIndices[index].value(ts));
  assert(0 <= metaIndex);
  assert(static_cast<size_t>(metaIndex) < _indices.size());
  if (metaIndex < _numActiveArcs.value(ts)) {
    return;
  }
  // swap with inactive last index:
  swap(ts, index);
  // make last inactive index active:
  _numActiveArcs.incValue(ts, 1);
}

void OutgoingDynamicArcContainer::makeInactive(Timestamp ts, size_t index) {
  assert(index < _metaIndices.size());
  const Int metaIndex = _metaIndices[index].value(ts);
  assert(static_cast<size_t>(metaIndex) < _indices.size());
  if (metaIndex >= _numActiveArcs.value(ts)) {
    return;
  }
  // make last active index inactive:
  _numActiveArcs.incValue(ts, -1);
  // swap with mewly inactive last index:
  swap(ts, index);
}

void OutgoingDynamicArcContainer::makeAllInactive(Timestamp ts) {
  _numActiveArcs.setValue(ts, 0);
}

bool OutgoingDynamicArcContainer::sanity(Timestamp ts) const {
  if (_arcs.size() != _indices.size() || _arcs.size() != _metaIndices.size()) {
    return false;
  }
  for (Int i = 0; i < static_cast<Int>(_arcs.size()); ++i) {
    const Int metaIndex = _metaIndices[i].value(ts);
    if (metaIndex < 0 || static_cast<Int>(_arcs.size()) <= metaIndex) {
      return false;
    }
    if (_indices[metaIndex].value(ts) != i) {
      return false;
    }
  }
  return true;
}

void OutgoingDynamicArcContainer::setNullId(size_t index) {
  _arcs[index].setInvariantNullId();
}

void OutgoingDynamicArcContainer::commitIf(Timestamp ts) {
  for (auto& inputIndex : _indices) {
    inputIndex.commitIf(ts);
  }
  for (auto& metaIndex : _metaIndices) {
    metaIndex.commitIf(ts);
  }
  _numActiveArcs.commitIf(ts);
}

OutgoingArcContainer::OutgoingArcContainer()
    : _outgoingStatic(), _outgoingDynamic(), _numArcs(0) {}

const std::vector<OutgoingArc>& OutgoingArcContainer::outgoingStatic() const {
  return _outgoingStatic;
}

const OutgoingDynamicArcContainer& OutgoingArcContainer::outgoingDynamic()
    const {
  return _outgoingDynamic;
}

size_t OutgoingArcContainer::numArcs() const { return _numArcs; }

bool OutgoingArcContainer::empty() const {
  return _outgoingStatic.empty() && _outgoingDynamic.empty();
}

void OutgoingArcContainer::emplaceStatic(LocalId localId,
                                         InvariantId invariantId) {
  _outgoingStatic.emplace_back(localId, invariantId);
  ++_numArcs;
}

void OutgoingArcContainer::emplaceDynamic(LocalId localId,
                                          InvariantId invariantId) {
  _outgoingDynamic.emplaceBack(localId, invariantId);
  ++_numArcs;
}

void OutgoingArcContainer::setDynamicNullId(size_t index,
                                            bool decreaseNumInputs) {
  _outgoingDynamic.setNullId(index);
  if (decreaseNumInputs) {
    --_numArcs;
  }
}

void OutgoingArcContainer::eraseStatic(size_t index, bool decreaseNumInputs) {
  _outgoingStatic.erase(_outgoingStatic.begin() + static_cast<Int>(index));
  if (decreaseNumInputs) {
    --_numArcs;
  }
}

void OutgoingArcContainer::makeDynamicActive(Timestamp ts, size_t index) {
  _outgoingDynamic.makeActive(ts, index);
}

void OutgoingArcContainer::makeDynamicInactive(Timestamp ts, size_t index) {
  _outgoingDynamic.makeInactive(ts, index);
}

void OutgoingArcContainer::makeAllDynamicInactive(Timestamp ts) {
  _outgoingDynamic.makeAllInactive(ts);
}

void OutgoingArcContainer::commitIf(Timestamp ts) {
  _outgoingDynamic.commitIf(ts);
}

}  // namespace var

}  // namespace atlantis::propagation