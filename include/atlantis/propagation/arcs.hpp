#pragma once

#include <vector>

#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

namespace invariant {

struct IncomingDynamicArc {
  VarId varId;
  size_t outgoingDynamicArcIndex;
};

class IncomingArcContainer {
  std::vector<VarId> _incomingStatic{};
  std::vector<IncomingDynamicArc> _incomingDynamic{};

 public:
  [[nodiscard]] size_t numArcs() const;

  [[nodiscard]] const std::vector<VarId>& incomingStatic() const;

  [[nodiscard]] const std::vector<IncomingDynamicArc>& incomingDynamic() const;

  LocalId emplaceStatic(VarId varId);

  LocalId emplaceDynamic(VarId varId, size_t outgoingDynamicArcIndex);
};

}  // namespace invariant

namespace var {
class OutgoingArc {
 private:
  LocalId _localId;
  InvariantId _invariantId;

 public:
  OutgoingArc(LocalId, InvariantId);

  [[nodiscard]] LocalId localId() const;
  [[nodiscard]] InvariantId invariantId() const;

  void setInvariantNullId();
};

class OutgoingDynamicArcContainer {
 private:
  std::vector<OutgoingArc> _arcs{};
  /*
   * _sparseIndices[val] = index of val in _sparseValues
   */
  std::vector<CommittableInt> _sparseIndices{};
  /*
   * _sparseValues[i] = index of i in _sparseIndices
   */
  std::vector<CommittableInt> _sparseValues{};
  CommittableInt _numActiveArcs{NULL_TIMESTAMP, 0};

  inline void swap(Timestamp ts, size_t index);

 public:
  [[nodiscard]] const std::vector<CommittableInt>& indices() const;

  [[nodiscard]] const std::vector<OutgoingArc>& arcs() const;

  [[nodiscard]] size_t size() const;

  [[nodiscard]] bool empty() const;

  [[nodiscard]] size_t numActive(Timestamp ts) const;

  [[nodiscard]] OutgoingArc& at(size_t index);

  [[nodiscard]] OutgoingArc& operator[](size_t index);

  void emplaceBack(LocalId localId, InvariantId invariantId);

  [[nodiscard]] bool isActive(Timestamp ts, size_t index) const;

  void makeActive(Timestamp ts, size_t index);

  void makeInactive(Timestamp ts, size_t index);

  void makeAllInactive(Timestamp);

  bool sanity(Timestamp ts) const;

  void setNullId(size_t index);

  bool isNullId(size_t index) const;

  void commitIf(Timestamp);
};

class OutgoingArcContainer {
 private:
  std::vector<OutgoingArc> _outgoingStatic;
  OutgoingDynamicArcContainer _outgoingDynamic;
  size_t _numArcs;

 public:
  OutgoingArcContainer();

  [[nodiscard]] const std::vector<OutgoingArc>& outgoingStatic() const;

  [[nodiscard]] const OutgoingDynamicArcContainer& outgoingDynamic() const;

  [[nodiscard]] size_t numArcs() const;

  [[nodiscard]] bool empty() const;

  void emplaceStatic(LocalId, InvariantId);

  void emplaceDynamic(LocalId, InvariantId);

  void setDynamicNullId(size_t index, bool decreaseNumInputs = false);

  void eraseStatic(size_t index, bool decreaseNumInputs = false);

  void makeDynamicActive(Timestamp ts, size_t index);

  void makeDynamicInactive(Timestamp ts, size_t index);

  void makeAllDynamicInactive(Timestamp);

  void commitIf(Timestamp);
};

}  // namespace var

}  // namespace atlantis::propagation