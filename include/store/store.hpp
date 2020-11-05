#pragma once

#include <memory>
#include <vector>

#include "core/engine.hpp"
#include "core/intVar.hpp"
#include "core/invariant.hpp"
class Store {
 private:
  IdMap<VarId, IntVar> m_intVars;

  IdMap<InvariantId, std::shared_ptr<Invariant>> m_invariants;

  IdMap<VarId, std::shared_ptr<IntView>> m_intViews;
  IdMap<VarId, VarId> m_intViewSourceId;

 public:
  Store(size_t estimatedSize, [[maybe_unused]] Id t_nullId)
      : m_intVars(estimatedSize),
        m_invariants(estimatedSize),
        m_intViews(estimatedSize),
        m_intViewSourceId(estimatedSize) {}

  [[nodiscard]] inline VarId createIntVar(Timestamp t, Int initValue,
                                          Int lowerBound, Int upperBound) {
    VarId newId = VarId(m_intVars.size() + 1, VarIdType::var);
    m_intVars.register_idx(newId,
                           IntVar(t, newId, initValue, lowerBound, upperBound));
    return newId;
  }
  [[nodiscard]] inline InvariantId createInvariantFromPtr(
      const std::shared_ptr<Invariant>& ptr) {
    auto newId = InvariantId(m_invariants.size() + 1);
    ptr->setId(newId);
    m_invariants.register_idx(newId, ptr);
    return newId;
  }

  [[nodiscard]] inline VarId createIntViewFromPtr(
      const std::shared_ptr<IntView>& ptr) {
    VarId newId = VarId(m_intViews.size() + 1, VarIdType::view);
    ptr->setId(newId);
    m_intViews.register_idx(newId, ptr);

    VarId parentId = ptr->getParentId();
    VarId source = parentId.idType == VarIdType::var
                       ? parentId
                       : m_intViewSourceId[parentId];
    m_intViewSourceId.register_idx(newId, source);
    return newId;
  }

  inline IntVar& getIntVar(VarId v) { return m_intVars[v]; }

  [[nodiscard]] inline const IntVar& getConstIntVar(VarId v) const {
    return m_intVars.at(v);
  }

  inline IntView& getIntView(VarId i) {
    assert(i.idType == VarIdType::view);
    return *(m_intViews[i.id]);
  }

  inline const std::shared_ptr<IntView> getConstIntView(VarId i) const {
    assert(i.idType == VarIdType::view);
    return (m_intViews.at(i.id));
  }

  inline VarId getIntViewSourceId(VarId v) {
    assert(v.idType == VarIdType::view);
    return m_intViewSourceId[v];
  }

  inline Invariant& getInvariant(InvariantId i) {
    return *(m_invariants[i.id]);
  }
  inline std::vector<IntVar>::iterator intVarBegin() {
    return m_intVars.begin();
  }
  inline std::vector<IntVar>::iterator intVarEnd() { return m_intVars.end(); }
  inline std::vector<std::shared_ptr<Invariant>>::iterator invariantBegin() {
    return m_invariants.begin();
  }
  inline std::vector<std::shared_ptr<Invariant>>::iterator invariantEnd() {
    return m_invariants.end();
  }

  [[nodiscard]] inline size_t getNumVariables() const {
    return m_intVars.size();
  }

  [[nodiscard]] inline size_t getNumInvariants() const {
    return m_invariants.size();
  }
};