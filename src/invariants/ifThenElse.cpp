#include "invariants/ifThenElse.hpp"

IfThenElse::IfThenElse(VarId b, VarId x, VarId y, VarId z)
    : Invariant(NULL_ID), m_b(b), m_xy({x, y}), m_z(z) {
  m_modifiedVars.reserve(1);
}

void IfThenElse::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(!m_id.equals(NULL_ID));

  registerDefinedVariable(e, m_z);
  e.registerInvariantDependsOnVar(m_id, m_b, 0);
  e.registerInvariantDependsOnVar(m_id, m_xy[0], 0);
  e.registerInvariantDependsOnVar(m_id, m_xy[1], 0);
}

// Non-branching implementation of the sign function
// @See: https://stackoverflow.com/a/4609795/3067688
void IfThenElse::recompute(Timestamp t, Engine& e) {
  auto b = e.getValue(t, m_b);
  updateValue(t, e, m_z,
    e.getValue(t, m_xy.at(
      std::max(0, (0 < b) - (b < 0))
    ))
  );
}

void IfThenElse::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  auto newValue = e.getValue(t, m_b);
  updateValue(t, e, m_z,
    e.getValue(t, m_xy.at(
      std::max(0, (0 < newValue) - (newValue < 0))
    ))
  );
}

VarId IfThenElse::getNextDependency(Timestamp t, Engine& e) {
  m_state.incValue(t, 1);
  if (m_state.getValue(t) == 0) {
    return m_b;
  } else if (m_state.getValue(t) == 1) {
    auto val = e.getValue(t, m_b);
    return m_xy.at(std::max(0, (0 < val) - (val < 0)));
  } else {
    return NULL_ID; // Done
  }
}

void IfThenElse::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  auto newValue = e.getValue(t, m_b);
  updateValue(t, e, m_z,
    e.getValue(t, m_xy.at(
      std::max(0, (0 < newValue) - (newValue < 0))
    ))
  );
}

void IfThenElse::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }