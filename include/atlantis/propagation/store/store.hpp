#pragma once

#include <memory>
#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/variables/intVar.hpp"
#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {

class Store {
 private:
  std::vector<IntVar> _intVars;
  std::vector<std::unique_ptr<Invariant>> _invariants;
  std::vector<std::unique_ptr<IntView>> _intViews;
  std::vector<VarId> _intViewSourceId;

 public:
  Store(size_t estimatedSize, [[maybe_unused]] size_t nullId)
      : _intVars(), _invariants(), _intViews(), _intViewSourceId() {
    _intVars.reserve(estimatedSize);
    _invariants.reserve(estimatedSize);
    _intViews.reserve(estimatedSize);
    _intViewSourceId.reserve(estimatedSize);
  }

  [[nodiscard]] inline VarViewId createIntVar(Timestamp ts, Int initValue,
                                              Int lowerBound, Int upperBound) {
    VarId vId(_intVars.size());
    const VarViewId newId = VarViewId(vId, false);
    _intVars.emplace_back(ts, vId, initValue, lowerBound, upperBound);
    return newId;
  }
  [[nodiscard]] inline InvariantId createInvariantFromPtr(
      std::unique_ptr<Invariant>&& ptr) {
    auto newId = InvariantId(_invariants.size());
    ptr->setId(newId);
    _invariants.emplace_back(std::move(ptr));
    return newId;
  }

  [[nodiscard]] inline VarViewId createIntViewFromPtr(
      std::unique_ptr<IntView>&& ptr) {
    const VarViewId newId = VarViewId(_intViews.size(), true);
    ptr->setId(ViewId(newId));
    const VarViewId parentId = ptr->parentId();
    const VarViewId source =
        parentId.isVar() ? parentId : _intViewSourceId[size_t(parentId)];
    _intViews.emplace_back(std::move(ptr));
    _intViewSourceId.emplace_back(VarId(source));
    return newId;
  }

  inline IntVar& intVar(VarId id) { return _intVars[id]; }

  [[nodiscard]] inline const IntVar& constIntVar(VarId id) const {
    return _intVars.at(id);
  }

  inline IntView& intView(ViewId id) { return *(_intViews[id]); }

  [[nodiscard]] const inline IntView& constIntView(ViewId id) const {
    return *(_intViews.at(id));
  }

  [[nodiscard]] inline VarId sourceId(VarViewId id) const noexcept {
    return id == NULL_ID
               ? NULL_ID
               : (id.isVar() ? VarId(id) : intViewSourceId(VarId(id)));
  }

  [[nodiscard]] inline VarId intViewSourceId(ViewId id) const {
    return _intViewSourceId.at(id);
  }

  inline Invariant& invariant(InvariantId invariantId) {
    return *(_invariants[invariantId]);
  }

  [[nodiscard]] inline const Invariant& constInvariant(
      InvariantId invariantId) const {
    return *(_invariants.at(invariantId));
  }

  inline std::vector<IntVar>::iterator intVarBegin() {
    return _intVars.begin();
  }
  inline std::vector<IntVar>::iterator intVarEnd() { return _intVars.end(); }
  inline std::vector<std::unique_ptr<Invariant>>::iterator invariantBegin() {
    return _invariants.begin();
  }
  inline std::vector<std::unique_ptr<Invariant>>::iterator invariantEnd() {
    return _invariants.end();
  }

  [[nodiscard]] inline size_t numVars() const { return _intVars.size(); }

  [[nodiscard]] inline size_t numInvariants() const {
    return _invariants.size();
  }

  [[nodiscard]] inline VarId dynamicInputVar(
      Timestamp ts, InvariantId invariantId) const noexcept {
    return sourceId(_invariants.at(invariantId)->dynamicInputVar(ts));
  }
};

}  // namespace atlantis::propagation
