#include "invariants/linear.hpp"

#include <vector>

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

Linear::Linear(std::vector<Int>&& A, std::vector<VarId>&& X, VarId b)
    : Invariant(NULL_ID),
      m_A(std::move(A)),
      m_X(std::move(X)),
      m_b(b),
      m_counter(NULL_TIMESTAMP, -1) {
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "constructing invariant"
            << "\n";
#endif
}

// Linear::Linear(Engine& e, std::vector<Int>&& A,
//                std::vector<std::shared_ptr<IntVar>>&& X,
//                std::shared_ptr<IntVar> b)
//     : Invariant(Engine::NULL_ID), m_A(std::move(A)), m_X(std::move(X)),
//     m_b(b) {
//   init(e);
// }

void Linear::init([[maybe_unused]] const Timestamp& t, Engine& e) {
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "initialising invariant " << m_id << "\n";
#endif
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);

  e.registerDefinedVariable(m_b, m_id);
  for (size_t i = 0; i < m_X.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_X[i], LocalId(i), m_A[i]);
  }
}

void Linear::recompute(const Timestamp& t, Engine& e) {
  Int sum = 0;
  for (size_t i = 0; i < m_X.size(); ++i) {
    sum += m_A[i] * e.getValue(t, m_X[i]);
  }
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Invariant " << m_id << " sum was recomputed to " << sum << "\n";
#endif
  // Dereference safe as incValue does not retain ptr.
  e.setValue(t, m_b, sum);
  m_counter.setValue(t, 0);
}

void Linear::notifyIntChanged(const Timestamp& t, Engine& e,
                              [[maybe_unused]] LocalId id, Int oldValue,
                              Int newValue, Int coef) {
  assert(newValue != oldValue);  // precondition
  e.incValue(t, m_b, (newValue - oldValue) * coef);

#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Invariant " << m_id << " notifying output changed by  "
            << (newValue - oldValue) * coef << "\n";
#endif
}

VarId Linear::getNextDependency(const Timestamp& t) {
  m_counter.incValue(t, 1);
  if (static_cast<size_t>(m_counter.getValue(t)) == m_X.size()) {
    return NULL_ID; // Done
  } else {
    return m_X.at(m_counter.getValue(t));
  }
}

void Linear::notifyCurrentDependencyChanged(const Timestamp& t, Engine& e, Int oldValue,
                                            Int newValue) {
  assert(m_counter.getValue(t) != -1);
  assert(newValue != oldValue);
  e.incValue(t, m_b, (newValue - oldValue) * m_A.at(m_counter.getValue(t)));
}

void Linear::commit(const Timestamp& t, Engine& e) {
  // todo: do nodes validate themself or is it done by engine?
  // this->validate(t);
  e.commitIf(t, m_b);
}