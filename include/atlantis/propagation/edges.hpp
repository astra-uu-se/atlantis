#pragma once

#include <vector>

#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

namespace invariant {

struct IncomingDynamicEdge {
  VarId varId;
  size_t outgoingDynamicEdgeIndex;
};

class IncomingEdgeContainer {
public:
  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = VarId;
    using pointer = VarId const*;
    using reference = const VarId&;

  private:
    const std::vector<VarId>& _incomingStatic;
    const std::vector<IncomingDynamicEdge>& _incomingDynamic{};
    size_t _index;

  public:
    explicit Iterator(const std::vector<VarId>& incomingStatic, const std::vector<IncomingDynamicEdge>& incomingDynamic, size_t index = 0);

    reference operator*() const;
    pointer operator->() const;

    Iterator& operator++();
    Iterator operator++(int);

    Iterator& operator--();
    Iterator operator--(int);

    Iterator& operator+=(size_t);
    Iterator& operator-=(size_t);

    Iterator operator+(size_t) const;
    Iterator operator-(size_t) const;

    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;
    [[nodiscard]] bool isDynamic() const noexcept;
  };
private:

  std::vector<VarId> _incomingStatic{};
  std::vector<IncomingDynamicEdge> _incomingDynamic{};

 public:
  [[nodiscard]] size_t numEdges() const;

  [[nodiscard]] const std::vector<VarId>& staticEdges() const;

  [[nodiscard]] VarId staticEdge(size_t index) const;

  [[nodiscard]] const std::vector<IncomingDynamicEdge>& dynamicEdges() const;

  [[nodiscard]] const IncomingDynamicEdge& dynamicEdge(size_t index) const;

  LocalId emplaceStatic(VarId varId);

  LocalId emplaceDynamic(VarId varId, size_t outgoingDynamicEdgeIndex);

  [[nodiscard]] Iterator begin() const;

  [[nodiscard]] Iterator end() const;
};

}  // namespace invariant

namespace var {
class OutgoingEdge {
  LocalId _localId;
  InvariantId _invariantId;

 public:
  OutgoingEdge(LocalId, InvariantId);

  [[nodiscard]] LocalId localId() const;
  [[nodiscard]] InvariantId invariantId() const;

  void setInvariantNullId();
};

class OutgoingDynamicEdgeContainer {
 private:
  std::vector<OutgoingEdge> _edges{};
  /*
   * _sparseIndices[val] = index of val in _sparseValues
   */
  std::vector<CommittableInt> _sparseIndices{};
  /*
   * _sparseValues[i] = index of i in _sparseIndices
   */
  std::vector<CommittableInt> _sparseValues{};
  CommittableInt _numActiveEdges{NULL_TIMESTAMP, 0};

  inline void swap(Timestamp ts, size_t index);

 public:
  [[nodiscard]] const std::vector<OutgoingEdge>& edges() const;

  [[nodiscard]] size_t size() const;

  [[nodiscard]] bool empty() const;

  [[nodiscard]] size_t numActive(Timestamp) const;

  [[nodiscard]] OutgoingEdge& at(size_t index);

  [[nodiscard]] const OutgoingEdge& at(size_t index) const;

  [[nodiscard]] OutgoingEdge& at(size_t index, Timestamp ts);

  [[nodiscard]] const OutgoingEdge& at(size_t index, Timestamp ts) const;

  [[nodiscard]] OutgoingEdge& operator[](size_t index);

  [[nodiscard]] const OutgoingEdge& operator[](size_t index) const;

  void emplaceBack(LocalId localId, InvariantId invariantId);

  [[nodiscard]] bool isActive(Timestamp ts, size_t index) const;

  void makeActive(Timestamp ts, size_t index);

  void makeInactive(Timestamp ts, size_t index);

  void makeAllInactive(Timestamp);

  [[nodiscard]] bool sanity(Timestamp ts) const;

  void setNullId(size_t index);

  [[nodiscard]] bool isNullId(size_t index) const;

  void commitIf(Timestamp);
};

class OutgoingEdgeContainer {
public:
  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = OutgoingEdge;
    using pointer = OutgoingEdge const*;
    using reference = const OutgoingEdge&;

  private:
    const std::vector<OutgoingEdge>& _outgoingStatic;
    const std::vector<OutgoingEdge>& _outgoingDynamic;
    size_t _index;
    explicit Iterator(const std::vector<OutgoingEdge>& outgoingStatic, const std::vector<OutgoingEdge>& outgoingDynamic, size_t index = 0);

  public:
    explicit Iterator(const std::vector<OutgoingEdge>&, const OutgoingDynamicEdgeContainer&, size_t index = 0);

    reference operator*() const;
    pointer operator->() const;

    Iterator& operator++();
    Iterator operator++(int);

    Iterator& operator--();
    Iterator operator--(int);

    Iterator& operator+=(size_t);
    Iterator& operator-=(size_t);

    Iterator operator+(size_t) const;
    Iterator operator-(size_t) const;

    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;
    [[nodiscard]] bool isDynamic() const noexcept;
  };
 private:
  std::vector<OutgoingEdge> _outgoingStatic;
  OutgoingDynamicEdgeContainer _outgoingDynamic;
  size_t _numEdges;

 public:
  OutgoingEdgeContainer();

  [[nodiscard]] const std::vector<OutgoingEdge>& staticEdges() const;

  [[nodiscard]] const OutgoingEdge& staticEdge(size_t) const;

  [[nodiscard]] const OutgoingDynamicEdgeContainer& dynamicEdges() const;

  [[nodiscard]] const OutgoingEdge& dynamicEdge(size_t) const;

  [[nodiscard]] const OutgoingEdge& dynamicEdge(size_t index, Timestamp ts) const;

  [[nodiscard]] size_t numEdges() const;

  [[nodiscard]] bool empty() const;

  void emplaceStatic(LocalId, InvariantId);

  void emplaceDynamic(LocalId, InvariantId);

  void setDynamicNullId(size_t index, bool decreaseNumInputs = false);

  void eraseStatic(size_t index, bool decreaseNumInputs = false);

  void makeDynamicActive(Timestamp ts, size_t index);

  void makeDynamicInactive(Timestamp ts, size_t index);

  void makeAllDynamicInactive(Timestamp);

  void commitIf(Timestamp);

  [[nodiscard]] Iterator begin() const;
  [[nodiscard]] Iterator end() const;
};

}  // namespace var

}  // namespace atlantis::propagation