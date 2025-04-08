#include "atlantis/propagation/store/store.hpp"

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/variables/intVar.hpp"
#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {

Store::Store() = default;

VarViewId Store::createIntVar(Timestamp ts, Int initValue, Int lowerBound,
                              Int upperBound) {
  VarId vId(_intVars.size());
  const VarViewId newId(vId, false);
  _intVars.emplace_back(ts, vId, initValue, lowerBound, upperBound);
  return newId;
}

InvariantId Store::createInvariantFromPtr(
    const std::shared_ptr<Invariant>& ptr) {
  const auto newId = InvariantId{_invariants.size()};
  ptr->setId(newId);
  _invariants.emplace_back(ptr);
  return newId;
}

VarViewId Store::createIntViewFromPtr(const std::shared_ptr<IntView>& ptr) {
  const VarViewId newId(_intViews.size(), true);
  ptr->setId(ViewId(newId));
  const VarViewId parentId = ptr->parentId();
  const VarViewId source =
      parentId.isVar() ? parentId : _intViewSourceId[size_t(parentId)];
  _intViews.emplace_back(ptr);
  _intViewSourceId.emplace_back(VarId(source));
  return newId;
}

IntVar& Store::intVar(VarId id) { return _intVars[id]; }

const IntVar& Store::constIntVar(VarId id) const { return _intVars.at(id); }

IntView& Store::intView(ViewId id) { return *(_intViews[id]); }

const IntView& Store::constIntView(ViewId id) const {
  return *(_intViews.at(id));
}

VarId Store::sourceId(VarViewId id) const noexcept {
  return id == NULL_ID ? NULL_ID
                       : (id.isVar() ? VarId(id) : intViewSourceId(VarId(id)));
}

VarId Store::intViewSourceId(ViewId id) const {
  return _intViewSourceId.at(id);
}

Invariant& Store::invariant(InvariantId invariantId) {
  return *(_invariants[invariantId]);
}

const Invariant& Store::constInvariant(InvariantId invariantId) const {
  return *(_invariants.at(invariantId));
}

std::vector<IntVar>::iterator Store::intVarBegin() { return _intVars.begin(); }

std::vector<IntVar>::iterator Store::intVarEnd() { return _intVars.end(); }

std::vector<std::shared_ptr<Invariant>>::iterator Store::invariantBegin() {
  return _invariants.begin();
}
std::vector<std::shared_ptr<Invariant>>::iterator Store::invariantEnd() {
  return _invariants.end();
}

size_t Store::numVars() const { return _intVars.size(); }

size_t Store::numInvariants() const { return _invariants.size(); }

VarId Store::dynamicInputVar(Timestamp ts,
                             InvariantId invariantId) const noexcept {
  return sourceId(_invariants.at(invariantId)->dynamicInputVar(ts));
}

}  // namespace atlantis::propagation
