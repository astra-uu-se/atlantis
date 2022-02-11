#pragma once

#include <memory>
#include <vector>

#include "invariants/invariant.hpp"
#include "utils/idMap.hpp"
#include "variables/intVar.hpp"
#include "views/intView.hpp"

class Store {
 private:
  IdMap<VarIdBase, IntVar> _intVars;

  IdMap<InvariantId, std::unique_ptr<Invariant>> _invariants;

  IdMap<VarIdBase, std::unique_ptr<IntView>> _intViews;
  IdMap<VarIdBase, VarId> _intViewSourceId;

 public:
  Store(size_t estimatedSize, [[maybe_unused]] Id nullId)
      : _intVars(estimatedSize),
        _invariants(estimatedSize),
        _intViews(estimatedSize),
        _intViewSourceId(estimatedSize) {}

  [[nodiscard]] inline VarId createIntVar(Timestamp ts, Int initValue,
                                          Int lowerBound, Int upperBound) {
    const VarId newId = VarId(_intVars.size() + 1, VarIdType::var);
    _intVars.register_idx(newId,
                          IntVar(ts, newId, initValue, lowerBound, upperBound));
    return newId;
  }
  [[nodiscard]] inline InvariantId createInvariantFromPtr(
      std::unique_ptr<Invariant>&& ptr) {
    auto newId = InvariantId(_invariants.size() + 1);
    ptr->setId(newId);
    _invariants.register_idx_move(newId, std::move(ptr));
    return newId;
  }

  [[nodiscard]] inline VarId createIntViewFromPtr(
      std::unique_ptr<IntView>&& ptr) {
    const VarId newId = VarId(_intViews.size() + 1, VarIdType::view);
    ptr->setId(newId);
    const VarId parentId = ptr->getParentId();
    const VarId source = parentId.idType == VarIdType::var
                             ? parentId
                             : _intViewSourceId[parentId];
    _intViews.register_idx_move(newId, std::move(ptr));
    _intViewSourceId.register_idx(newId, source);
    return newId;
  }

  inline IntVar& getIntVar(VarId id) { return _intVars[id]; }

  [[nodiscard]] inline const IntVar& getConstIntVar(VarId id) const {
    return _intVars.at(id);
  }

  inline IntView& getIntView(VarId id) {
    assert(id.idType == VarIdType::view);
    return *(_intViews[id.id]);
  }

  [[nodiscard]] const inline IntView& getConstIntView(VarId id) const {
    assert(id.idType == VarIdType::view);
    return *(_intViews.at(id.id));
  }

  inline VarId getIntViewSourceId(VarId id) const {
    assert(id.idType == VarIdType::view);
    return _intViewSourceId.at(id);
  }

  inline Invariant& getInvariant(InvariantId invariantId) {
    return *(_invariants[invariantId.id]);
  }

  inline const Invariant& getConstInvariant(InvariantId invariantId) const {
    return *(_invariants.at(invariantId.id));
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

  [[nodiscard]] inline size_t getNumVariables() const {
    return _intVars.size();
  }

  [[nodiscard]] inline size_t getNumInvariants() const {
    return _invariants.size();
  }
};