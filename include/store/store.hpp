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

 public:
  Store(size_t estimatedSize, [[maybe_unused]] Id t_nullId)
      : m_intVars(estimatedSize), m_invariants(estimatedSize) {}
 
  [[nodiscard]] inline VarId createIntVar(Timestamp t, Int initValue,
                                          Int lowerBound, Int upperBound) {
    VarId newId = VarId(m_intVars.size()+1);
    m_intVars.register_idx(newId,
                           IntVar(t, newId, initValue, lowerBound, upperBound));
    return newId;
  }
  [[nodiscard]] inline InvariantId createInvariantFromPtr(
      std::shared_ptr<Invariant> ptr) {
    InvariantId newId = InvariantId(m_invariants.size() + 1);
    ptr->setId(newId);
    m_invariants.register_idx(newId, ptr);
    return newId;
  }
  inline IntVar& getIntVar(VarId v) { return m_intVars[v]; }

  inline const IntVar& getConstIntVar(VarId v) const { return m_intVars.at(v); }

  inline Invariant& getInvariant(InvariantId& i) {
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

  inline size_t getNumVariables() const { return m_intVars.size(); }

  inline size_t getNumInvariants() const { return m_invariants.size(); }
};