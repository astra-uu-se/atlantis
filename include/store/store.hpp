#pragma once

#include <memory>
#include <vector>

#include "core/engine.hpp"
#include "core/intVar.hpp"
#include "core/invariant.hpp"
class Store {
 private:
  std::vector<IntVar> m_intVars;
  std::vector<size_t> m_intVarIndexMap;
  std::vector<std::shared_ptr<IntVarView>> m_intVarViews;
  std::vector<std::shared_ptr<Invariant>> m_invariants;

 public:
  Store(size_t estimatedSize, [[maybe_unused]] Id t_nullId) {
    m_intVars.reserve(estimatedSize);
    m_intVarIndexMap.reserve(estimatedSize);
    m_intVarViews.reserve(estimatedSize);
    m_invariants.reserve(estimatedSize);

    m_intVars.emplace_back(0, 0);
    m_intVarIndexMap.push_back(-1);
    m_intVarViews.push_back(nullptr);
    m_invariants.push_back(nullptr);
  }
  [[nodiscard]] inline VarId createIntVar(Timestamp t, Int initValue, Int lowerBound, Int upperBound) {
    VarId newId = VarId(m_intVars.size());
    m_intVars.emplace_back(IntVar(t, newId, initValue, lowerBound, upperBound));
    m_intVarIndexMap.push_back(newId);
    return newId;
  }
  [[nodiscard]] inline InvariantId createInvariantFromPtr(
      std::shared_ptr<Invariant> ptr) {
    InvariantId newId = InvariantId(m_invariants.size());
    ptr->setId(newId);
    m_invariants.push_back(ptr);
    return newId;
  }
  [[nodiscard]] inline VarId createIntViewFromPtr(
      std::shared_ptr<IntVarView> ptr) {
    VarId newId = VarId(m_intVarViews.size(), VarIdType::view);
    ptr->setId(newId);
    m_intVarViews.push_back(ptr);
    return newId;
  }
  inline IntVar& getIntVar(VarId& v) {
    return m_intVars.at(m_intVarIndexMap.at(v.id));
  }

  inline const IntVar& getConstIntVar(VarId& v) const {
    return m_intVars.at(m_intVarIndexMap.at(v.id));
  }

  inline IntVarView& getIntVarView(const VarId& i) const {
    return *(m_intVarViews.at(i.id));
  }

  inline Invariant& getInvariant(InvariantId& i) const {
    return *(m_invariants.at(i.id));
  }
  inline std::vector<IntVar>::iterator intVarBegin() { return m_intVars.begin() + 1; }
  inline std::vector<IntVar>::iterator intVarEnd() { return m_intVars.end(); }
  inline std::vector<std::shared_ptr<Invariant>>::iterator invariantBegin() {
    return m_invariants.begin() + 1;
  }
  inline std::vector<std::shared_ptr<Invariant>>::iterator invariantEnd() {
    return m_invariants.end();
  }

  inline std::vector<std::shared_ptr<IntVarView>>::iterator intVarViewBegin() {
    return m_intVarViews.begin() + 1;
  }
  inline std::vector<std::shared_ptr<IntVarView>>::iterator intVarViewEnd() {
    return m_intVarViews.end();
  }

  inline size_t getNumVariables() const { return m_intVars.size() - 1; }

  inline size_t getNumInvariants() const { return m_invariants.size() - 1; }
};