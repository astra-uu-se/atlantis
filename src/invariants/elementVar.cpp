#include "invariants/elementVar.hpp"

#include <vector>

#include "core/engine.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

ElementVar::ElementVar(VarId i, std::vector<VarId>&& X, VarId b)
    : Invariant(NULL_ID), m_i(i), m_X(std::move(X)), m_b(b) {}

void ElementVar::init([[maybe_unused]] const Timestamp& t, Engine& e) {
  assert(m_id != NULL_ID);

  e.registerDefinedVariable(m_b, m_id);
  e.registerInvariantDependsOnVar(m_id, m_i, LocalId(-1), 0);
  for (size_t i = 0; i < m_X.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_X[i], LocalId(i), 0);
  }
}

void ElementVar::recompute(const Timestamp& t, Engine& e) {
  e.setValue(t, m_b, e.getValue(t, m_X.at(e.getValue(t, m_i))));
}

void ElementVar::notifyIntChanged(const Timestamp& t, Engine& e, LocalId id,
                                  Int oldValue, Int newValue, Int) {
  assert(newValue != oldValue);
  if (id.id == LocalId(-1).id) {
    e.setValue(t, m_b, e.getValue(t, m_X.at(newValue)));
  } else if (id.id == static_cast<IdBase>(e.getValue(t, m_i))) {
    e.setValue(t, m_b, newValue);
  }
  // e.setValue(t, m_b, m_A.at(newValue));
}

VarId ElementVar::getNextDependency(const Timestamp& t, Engine& e) {
  m_state.incValue(t, 1);
  if (m_state.getValue(t) == 0) {
    return m_i;
  } else if (m_state.getValue(t) == 1) {
    return m_X.at(e.getValue(t, m_i));
  } else {
    return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentDependencyChanged(const Timestamp& t, Engine& e) {
  assert(m_state.getValue(t) == 0 || m_state.getValue(t) == 1);
  e.setValue(t, m_b, e.getValue(t, m_X.at(e.getValue(t, m_i))));
}

void ElementVar::commit(const Timestamp& t, Engine& e) {
  // todo: do nodes validate themself or is it done by engine?
  // this->validate(t);
  e.commitIf(t, m_b);
}