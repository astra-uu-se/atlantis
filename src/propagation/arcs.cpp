#include "atlantis/propagation/arcs.hpp"

#include <cassert>

#include "atlantis/exceptions/exceptions.hpp"

namespace atlantis::propagation {

namespace invariant {

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
                                             size_t outgoingDynamicArcIndex) {
  if (!_incomingStatic.empty()) {
    // All dynamic input variables must be registered before registering
    // static input variables.
    throw OutOfOrderIndexRegistration();
  }
  _incomingDynamic.emplace_back(varId, outgoingDynamicArcIndex);
  return static_cast<LocalId>(_incomingDynamic.size() - 1);
}

}  // namespace invariant

namespace var {
OutgoingArc::OutgoingArc(const LocalId localId, const InvariantId invariantId)
    : _localId(localId), _invariantId(invariantId) {}

LocalId OutgoingArc::localId() const { return _localId; }
InvariantId OutgoingArc::invariantId() const { return _invariantId; }

void OutgoingArc::setInvariantNullId() { _invariantId = NULL_ID; }

void OutgoingDynamicArcContainer::swap(Timestamp ts, size_t index1) {
  // move val1 at index1 to the end of the active indices
  // sanity:
  assert(index1 < _sparseValues.size());
  assert(0 <= _numActiveArcs.value(ts));
  assert(static_cast<size_t>(_numActiveArcs.value(ts)) < _sparseIndices.size());
  // get the value at index1:
  const Int val1 = _sparseValues[index1].value(ts);
  assert(0 <= val1);
  assert(static_cast<size_t>(val1) < _sparseIndices.size());
  // get the last active value:
  const Int val2 = _numActiveArcs.value(ts);
  assert(0 <= val2);
  assert(static_cast<size_t>(val2) < _sparseIndices.size());
  if (val1 == val2) {
    return;
  }
  // get the index of the last active value:
  const size_t index2 = static_cast<size_t>(_sparseIndices[val2].value(ts));
  assert(index2 < _sparseIndices.size());
  // perform index swap:
  _sparseIndices[val1].setValue(ts, index2);
  _sparseIndices[val2].setValue(ts, index1);
  // perform meta index swap:
  _sparseValues[index1].setValue(ts, val2);
  _sparseValues[index2].setValue(ts, val1);
}

const std::vector<CommittableInt>& OutgoingDynamicArcContainer::indices()
    const {
  return _sparseIndices;
}

const std::vector<OutgoingArc>& OutgoingDynamicArcContainer::arcs() const {
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
  assert(invariantId != NULL_ID);
  _arcs.emplace_back(localId, invariantId);
  _sparseIndices.push_back(
      CommittableInt(NULL_TIMESTAMP, static_cast<Int>(_arcs.size()) - 1));
  _sparseValues.push_back(
      CommittableInt(NULL_TIMESTAMP, static_cast<Int>(_arcs.size()) - 1));
}

bool OutgoingDynamicArcContainer::isActive(Timestamp ts, size_t index) const {
  assert(index < _sparseValues.size());
  return _sparseValues[index].value(ts) < _numActiveArcs.value(ts);
}

void OutgoingDynamicArcContainer::makeActive(Timestamp ts, size_t index) {
  assert(index < _sparseValues.size());
  const Int val = static_cast<size_t>(_sparseValues[index].value(ts));
  assert(0 <= val);
  assert(static_cast<size_t>(val) < _sparseIndices.size());
  if (val < _numActiveArcs.value(ts)) {
    return;
  }
  // swap with inactive last index:
  swap(ts, index);
  // make last inactive index active:
  _numActiveArcs.incValue(ts, 1);
}

void OutgoingDynamicArcContainer::makeInactive(Timestamp ts, size_t index) {
  assert(index < _sparseValues.size());
  const Int metaIndex = _sparseValues[index].value(ts);
  assert(static_cast<size_t>(metaIndex) < _sparseIndices.size());
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
  if (_arcs.size() != _sparseIndices.size() ||
      _arcs.size() != _sparseValues.size()) {
    return false;
  }
  for (Int index = 0; index < static_cast<Int>(_arcs.size()); ++index) {
    const Int val = _sparseValues[index].value(ts);
    if (val < 0 || static_cast<Int>(_arcs.size()) <= val) {
      return false;
    }
    if (_sparseIndices[val].value(ts) != index) {
      return false;
    }
  }
  return true;
}

void OutgoingDynamicArcContainer::setNullId(size_t index) {
  _arcs[index].setInvariantNullId();
}

bool OutgoingDynamicArcContainer::isNullId(size_t index) const {
  return _arcs[index].invariantId() == NULL_ID;
}

void OutgoingDynamicArcContainer::commitIf(Timestamp ts) {
  assert(static_cast<int>(_numActiveArcs.tmpTimestamp() == ts) >=
         std::max<int>(static_cast<int>(std::any_of(
                           _sparseIndices.begin(), _sparseIndices.end(),
                           [ts](const CommittableInt& index) {
                             return index.tmpTimestamp() == ts;
                           })),
                       static_cast<int>(std::any_of(
                           _sparseValues.begin(), _sparseValues.end(),
                           [ts](const CommittableInt& index) {
                             return index.tmpTimestamp() == ts;
                           }))));
  for (auto& inputIndex : _sparseIndices) {
    inputIndex.commitIf(ts);
  }
  for (auto& metaIndex : _sparseValues) {
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
  assert(invariantId != NULL_ID);
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