#include "atlantis/propagation/edges.hpp"

#include <algorithm>
#include <boost/multi_index_container.hpp>
#include <cassert>

#include "atlantis/exceptions/exceptions.hpp"

namespace atlantis::propagation {

namespace invariant {

IncomingEdgeContainer::Iterator::Iterator(const std::vector<VarId>& incomingStatic, const std::vector<IncomingDynamicEdge>& incomingDynamic, const size_t index) :
_incomingStatic(incomingStatic), _incomingDynamic(incomingDynamic), _index(index) {}

IncomingEdgeContainer::Iterator::reference
IncomingEdgeContainer::Iterator::operator*() const {
  return _index < _incomingStatic.size() ? _incomingStatic[_index] : _incomingDynamic[_index - _incomingStatic.size()].varId;
}
IncomingEdgeContainer::Iterator::pointer
IncomingEdgeContainer::Iterator::operator->() const {
  return &(_index < _incomingStatic.size() ? _incomingStatic[_index] : _incomingDynamic[_index - _incomingStatic.size()].varId);
}
IncomingEdgeContainer::Iterator& IncomingEdgeContainer::Iterator::operator++() {
  ++_index;
  return *this;
}
IncomingEdgeContainer::Iterator IncomingEdgeContainer::Iterator::operator++(int) {
  const Iterator tmp(*this);
  ++_index;
  return tmp;
}

IncomingEdgeContainer::Iterator& IncomingEdgeContainer::Iterator::operator--() {
  --_index;
  return *this;
}
IncomingEdgeContainer::Iterator IncomingEdgeContainer::Iterator::operator--(int) {
  const Iterator tmp(*this);
  --_index;
  return tmp;
}
IncomingEdgeContainer::Iterator& IncomingEdgeContainer::Iterator::operator+=(
    const size_t offset) {
  _index+=offset;
  return *this;
}
IncomingEdgeContainer::Iterator& IncomingEdgeContainer::Iterator::operator-=(
    const size_t offset) {
  _index-=offset;
  return *this;
}

IncomingEdgeContainer::Iterator IncomingEdgeContainer::Iterator::operator+(
    const size_t offset) const {
  return Iterator(_incomingStatic, _incomingDynamic, _index + offset);
}

IncomingEdgeContainer::Iterator IncomingEdgeContainer::Iterator::operator-(
    const size_t offset) const {
  return Iterator(_incomingStatic, _incomingDynamic, _index - offset);
}

bool IncomingEdgeContainer::Iterator::operator==(const Iterator& other) const {
  return &_incomingStatic == &other._incomingStatic && &_incomingDynamic == &other._incomingDynamic && _index == other._index;
}

bool IncomingEdgeContainer::Iterator::operator!=(const Iterator& other) const {
  return &_incomingStatic != &other._incomingStatic || &_incomingDynamic != &other._incomingDynamic || _index != other._index;
}

bool IncomingEdgeContainer::Iterator::isDynamic() const noexcept {
  return _index >= _incomingStatic.size() && _index < _incomingStatic.size() + _incomingDynamic.size();
}

size_t IncomingEdgeContainer::numEdges() const {
  return _incomingStatic.size() + _incomingDynamic.size();
}

const std::vector<VarId>& IncomingEdgeContainer::staticEdges() const {
  return _incomingStatic;
}

VarId IncomingEdgeContainer::staticEdge(const size_t index) const {
  assert(index < _incomingStatic.size());
  return _incomingStatic[index];
}

const std::vector<IncomingDynamicEdge>& IncomingEdgeContainer::dynamicEdges()
    const {
  return _incomingDynamic;
}

const IncomingDynamicEdge& IncomingEdgeContainer::dynamicEdge(
    const size_t index) const {
  assert(index < _incomingDynamic.size());
  return _incomingDynamic[index];
}

LocalId IncomingEdgeContainer::emplaceStatic(VarId varId) {
  _incomingStatic.emplace_back(varId);
  return static_cast<LocalId>(numEdges() - 1);
}

LocalId IncomingEdgeContainer::emplaceDynamic(VarId varId,
                                             size_t outgoingDynamicEdgeIndex) {
  if (!_incomingStatic.empty()) {
    // All dynamic input variables must be registered before registering
    // static input variables.
    throw OutOfOrderIndexRegistration();
  }
  _incomingDynamic.emplace_back(varId, outgoingDynamicEdgeIndex);
  return _incomingDynamic.size() - 1;
}

IncomingEdgeContainer::Iterator IncomingEdgeContainer::begin() const {
  return Iterator(_incomingStatic, _incomingDynamic, 0);
}

IncomingEdgeContainer::Iterator IncomingEdgeContainer::end() const {
  return Iterator(_incomingStatic, _incomingDynamic,  _incomingStatic.size() + _incomingDynamic.size());
}
}  // namespace invariant

namespace var {
OutgoingEdge::OutgoingEdge(const LocalId localId, const InvariantId invariantId)
    : _localId(localId), _invariantId(invariantId) {}

LocalId OutgoingEdge::localId() const { return _localId; }
InvariantId OutgoingEdge::invariantId() const { return _invariantId; }

void OutgoingEdge::setInvariantNullId() { _invariantId = NULL_ID; }

void OutgoingDynamicEdgeContainer::swap(const Timestamp ts, size_t index1) {
  // move val1 at index1 to the end of the active indices
  // sanity:
  assert(index1 < _sparseValues.size());
  assert(0 <= _numActiveEdges.value(ts));
  assert(static_cast<size_t>(_numActiveEdges.value(ts)) < _sparseIndices.size());
  // get the value at index1:
  const Int val1 = _sparseValues[index1].value(ts);
  assert(0 <= val1);
  assert(static_cast<size_t>(val1) < _sparseIndices.size());
  // get the last active value:
  const Int val2 = _numActiveEdges.value(ts);
  assert(0 <= val2);
  assert(static_cast<size_t>(val2) < _sparseIndices.size());
  if (val1 == val2) {
    return;
  }
  // get the index of the last active value:
  const auto index2 = static_cast<size_t>(_sparseIndices[val2].value(ts));
  assert(index2 < _sparseIndices.size());
  // perform index swap:
  _sparseIndices[val1].setValue(ts, static_cast<Int>(index2));
  _sparseIndices[val2].setValue(ts, static_cast<Int>(index1));
  // perform meta index swap:
  _sparseValues[index1].setValue(ts, val2);
  _sparseValues[index2].setValue(ts, val1);
}

const std::vector<OutgoingEdge>& OutgoingDynamicEdgeContainer::edges() const {
  return _edges;
}

size_t OutgoingDynamicEdgeContainer::size() const { return _edges.size(); }

bool OutgoingDynamicEdgeContainer::empty() const { return _edges.empty(); }

size_t OutgoingDynamicEdgeContainer::numActive(const Timestamp ts) const {
  return _numActiveEdges.value(ts);
}

OutgoingEdge& OutgoingDynamicEdgeContainer::at(const size_t index) {
  return _edges.at(index);
}

const OutgoingEdge& OutgoingDynamicEdgeContainer::at(const size_t index) const {
  return _edges.at(index);
}

OutgoingEdge& OutgoingDynamicEdgeContainer::at(const size_t index, const Timestamp ts) {
  assert(index < _edges.size());
  const auto sparseIndex = static_cast<size_t>(_sparseIndices[index].value(ts));
  assert(sparseIndex < _edges.size());
  return _edges[sparseIndex];
}

OutgoingEdge& OutgoingDynamicEdgeContainer::operator[](const size_t index) {
  assert(index < _edges.size());
  return _edges[index];
}

const OutgoingEdge& OutgoingDynamicEdgeContainer::at(const size_t index, const Timestamp ts) const {
  assert(index < _edges.size());
  const auto sparseIndex = static_cast<size_t>(_sparseIndices[index].value(ts));
  assert(sparseIndex < _edges.size());
  return _edges[sparseIndex];
}

const OutgoingEdge& OutgoingDynamicEdgeContainer::operator[](
    const size_t index) const {
  assert(index < _edges.size());
  return _edges[index];
}

void OutgoingDynamicEdgeContainer::emplaceBack(LocalId localId,
                                              InvariantId invariantId) {
  assert(invariantId != NULL_ID);
  _edges.emplace_back(localId, invariantId);
  _sparseIndices.emplace_back(NULL_TIMESTAMP, static_cast<Int>(_edges.size()) - 1);
  _sparseValues.emplace_back(NULL_TIMESTAMP, static_cast<Int>(_edges.size()) - 1);
}

bool OutgoingDynamicEdgeContainer::isActive(const Timestamp ts,
                                           const size_t index) const {
  assert(index < _sparseValues.size());
  return _sparseValues[index].value(ts) < _numActiveEdges.value(ts);
}

void OutgoingDynamicEdgeContainer::makeActive(const Timestamp ts,
                                             const size_t index) {
  assert(index < _sparseValues.size());
  const Int val = _sparseValues[index].value(ts);
  assert(0 <= val);
  assert(static_cast<size_t>(val) < _sparseIndices.size());
  if (val < _numActiveEdges.value(ts)) {
    return;
  }
  // swap with inactive last index:
  swap(ts, index);
  // make last inactive index active:
  _numActiveEdges.incValue(ts, 1);
}

void OutgoingDynamicEdgeContainer::makeInactive(const Timestamp ts,
                                                const size_t index) {
  assert(index < _sparseValues.size());
  const Int metaIndex = _sparseValues[index].value(ts);
  assert(static_cast<size_t>(metaIndex) < _sparseIndices.size());
  if (metaIndex >= _numActiveEdges.value(ts)) {
    return;
  }
  // make last active index inactive:
  _numActiveEdges.incValue(ts, -1);
  // swap with newly inactive last index:
  swap(ts, index);
}

void OutgoingDynamicEdgeContainer::makeAllInactive(const Timestamp ts) {
  _numActiveEdges.setValue(ts, 0);
}

bool OutgoingDynamicEdgeContainer::sanity(const Timestamp ts) const {
  if (_edges.size() != _sparseIndices.size() ||
      _edges.size() != _sparseValues.size()) {
    return false;
  }
  for (Int index = 0; index < static_cast<Int>(_edges.size()); ++index) {
    const Int val = _sparseValues[index].value(ts);
    if (val < 0 || static_cast<Int>(_edges.size()) <= val) {
      return false;
    }
    if (_sparseIndices[val].value(ts) != index) {
      return false;
    }
  }
  return true;
}

void OutgoingDynamicEdgeContainer::setNullId(const size_t index) {
  _edges[index].setInvariantNullId();
}

bool OutgoingDynamicEdgeContainer::isNullId(const size_t index) const {
  return _edges[index].invariantId() == NULL_ID;
}

void OutgoingDynamicEdgeContainer::commitIf(const Timestamp ts) {
  assert(static_cast<int>(_numActiveEdges.tmpTimestamp() == ts) >=
         std::max<int>(static_cast<int>(std::ranges::any_of(
                           _sparseIndices,
                           [ts](const CommittableInt& index) {
                             return index.tmpTimestamp() == ts;
                           })),
                       static_cast<int>(std::ranges::any_of(
                           _sparseValues,
                           [ts](const CommittableInt& index) {
                             return index.tmpTimestamp() == ts;
                           }))));
  for (auto& inputIndex : _sparseIndices) {
    inputIndex.commitIf(ts);
  }
  for (auto& metaIndex : _sparseValues) {
    metaIndex.commitIf(ts);
  }
  _numActiveEdges.commitIf(ts);
}

OutgoingEdgeContainer::OutgoingEdgeContainer()
    : _numEdges(0) {}

const std::vector<OutgoingEdge>& OutgoingEdgeContainer::staticEdges() const {
  return _outgoingStatic;
}
const OutgoingEdge& OutgoingEdgeContainer::staticEdge(const size_t index) const {
  assert(index < _outgoingStatic.size());
  return _outgoingStatic[index];
}

const OutgoingEdge& OutgoingEdgeContainer::dynamicEdge(const size_t index)
    const {
  assert(index < _outgoingDynamic.size());
  return _outgoingDynamic[index];
}

const OutgoingEdge& OutgoingEdgeContainer::dynamicEdge(const size_t index, const Timestamp ts)
    const {
  assert(index < _outgoingDynamic.size());
  return _outgoingDynamic.at(index, ts);
}

const OutgoingDynamicEdgeContainer& OutgoingEdgeContainer::dynamicEdges()
    const {
  return _outgoingDynamic;
}

size_t OutgoingEdgeContainer::numEdges() const { return _numEdges; }

bool OutgoingEdgeContainer::empty() const {
  return _outgoingStatic.empty() && _outgoingDynamic.empty();
}

void OutgoingEdgeContainer::emplaceStatic(const LocalId localId,
                                         const InvariantId invariantId) {
  assert(invariantId != NULL_ID);
  _outgoingStatic.emplace_back(localId, invariantId);
  ++_numEdges;
}

void OutgoingEdgeContainer::emplaceDynamic(const LocalId localId,
                                          const InvariantId invariantId) {
  assert(invariantId != NULL_ID);
  _outgoingDynamic.emplaceBack(localId, invariantId);
  ++_numEdges;
}

void OutgoingEdgeContainer::setDynamicNullId(const size_t index,
                                            const bool decreaseNumInputs) {
  _outgoingDynamic.setNullId(index);
  if (decreaseNumInputs) {
    --_numEdges;
  }
}

void OutgoingEdgeContainer::eraseStatic(const size_t index, const bool decreaseNumInputs) {
  _outgoingStatic.erase(_outgoingStatic.begin() + static_cast<Int>(index));
  if (decreaseNumInputs) {
    --_numEdges;
  }
}

void OutgoingEdgeContainer::makeDynamicActive(const Timestamp ts, const size_t index) {
  _outgoingDynamic.makeActive(ts, index);
}

void OutgoingEdgeContainer::makeDynamicInactive(const Timestamp ts, const size_t index) {
  _outgoingDynamic.makeInactive(ts, index);
}

void OutgoingEdgeContainer::makeAllDynamicInactive(const Timestamp ts) {
  _outgoingDynamic.makeAllInactive(ts);
}

void OutgoingEdgeContainer::commitIf(const Timestamp ts) {
  _outgoingDynamic.commitIf(ts);
}
OutgoingEdgeContainer::Iterator OutgoingEdgeContainer::begin() const {
  return Iterator(_outgoingStatic, _outgoingDynamic);
}
OutgoingEdgeContainer::Iterator OutgoingEdgeContainer::end() const {
  return Iterator(_outgoingStatic, _outgoingDynamic, _outgoingStatic.size() + _outgoingDynamic.size());
}

OutgoingEdgeContainer::Iterator::Iterator(const std::vector<OutgoingEdge>& outgoingStatic, const OutgoingDynamicEdgeContainer& outgoingDynamic, const size_t index)
: Iterator(outgoingStatic, outgoingDynamic.edges(), index) {}

OutgoingEdgeContainer::Iterator::Iterator(const std::vector<OutgoingEdge>& outgoingStatic, const std::vector<OutgoingEdge>& outgoingDynamic, const size_t index)
: _outgoingStatic(outgoingStatic), _outgoingDynamic(outgoingDynamic), _index(index) {}

OutgoingEdgeContainer::Iterator::reference
OutgoingEdgeContainer::Iterator::operator*() const {
  return _index < _outgoingStatic.size() ? _outgoingStatic[_index] : _outgoingDynamic[_index - _outgoingStatic.size()];
}
OutgoingEdgeContainer::Iterator::pointer
OutgoingEdgeContainer::Iterator::operator->() const {
  return &(_index < _outgoingStatic.size() ? _outgoingStatic[_index] : _outgoingDynamic[_index - _outgoingStatic.size()]);
}
OutgoingEdgeContainer::Iterator& OutgoingEdgeContainer::Iterator::operator++() {
  ++_index;
  return *this;
}
OutgoingEdgeContainer::Iterator OutgoingEdgeContainer::Iterator::operator++(int) {
  const Iterator tmp(*this);
  ++_index;
  return tmp;
}

OutgoingEdgeContainer::Iterator& OutgoingEdgeContainer::Iterator::operator--() {
  --_index;
  return *this;
}
OutgoingEdgeContainer::Iterator OutgoingEdgeContainer::Iterator::operator--(int) {
  const Iterator tmp(*this);
  --_index;
  return tmp;
}
OutgoingEdgeContainer::Iterator& OutgoingEdgeContainer::Iterator::operator+=(
    const size_t offset) {
  _index+=offset;
  return *this;
}
OutgoingEdgeContainer::Iterator& OutgoingEdgeContainer::Iterator::operator-=(
    const size_t offset) {
  _index-=offset;
  return *this;
}

OutgoingEdgeContainer::Iterator OutgoingEdgeContainer::Iterator::operator+(
    const size_t offset) const {
  return Iterator(_outgoingStatic, _outgoingDynamic, _index + offset);
}

OutgoingEdgeContainer::Iterator OutgoingEdgeContainer::Iterator::operator-(
    const size_t offset) const {
  return Iterator(_outgoingStatic, _outgoingDynamic, _index - offset);
}

bool OutgoingEdgeContainer::Iterator::operator==(const Iterator& other) const {
  return &_outgoingStatic == &other._outgoingStatic && &_outgoingDynamic == &other._outgoingDynamic && _index == other._index;
}

bool OutgoingEdgeContainer::Iterator::operator!=(const Iterator& other) const {
  return &_outgoingStatic != &other._outgoingStatic || &_outgoingDynamic != &other._outgoingDynamic || _index != other._index;
}

bool OutgoingEdgeContainer::Iterator::isDynamic() const noexcept {
  return _index >= _outgoingStatic.size() && _index < _outgoingStatic.size() + _outgoingDynamic.size();
}

}  // namespace var

}  // namespace atlantis::propagation