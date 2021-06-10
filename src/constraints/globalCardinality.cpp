#include "constraints/globalCardinality.hpp"

#include "core/engine.hpp"
#include "variables/savedInt.hpp"
/**
 * @param violationId id for the violationCount
 */
GlobalCardinality::GlobalCardinality(VarId violationId,
                                     std::vector<VarId> t_variables,
                                     std::vector<Int> cover,
                                     std::vector<Int> t_counts)
    : GlobalCardinality(violationId, t_variables, cover, t_counts, t_counts) {}

GlobalCardinality::GlobalCardinality(VarId violationId,
                                     std::vector<VarId> t_variables,
                                     std::vector<Int> cover,
                                     std::vector<Int> t_counts, bool closed)
    : GlobalCardinality(violationId, t_variables, cover, t_counts, t_counts,
                        closed) {}

GlobalCardinality::GlobalCardinality(VarId violationId,
                                     std::vector<VarId> t_variables,
                                     std::vector<Int> cover,
                                     std::vector<Int> lowerBound,
                                     std::vector<Int> upperBound)
    : GlobalCardinality(violationId, t_variables, cover, lowerBound, upperBound,
                        false) {}

GlobalCardinality::GlobalCardinality(VarId violationId,
                                     std::vector<VarId> t_variables,
                                     std::vector<Int> cover,
                                     std::vector<Int> lowerBound,
                                     std::vector<Int> upperBound, bool closed)
    : Constraint(NULL_ID, violationId),
      m_variables(std::move(t_variables)),
      m_cover(std::move(cover)),
      m_lowerBound(),
      m_upperBound(),
      m_localValues(),
      m_excess(NULL_TIMESTAMP, 0),
      m_shortage(NULL_TIMESTAMP, 0),
      m_counts(),
      m_offset(0) {
  m_closed = closed ? 0 : 1;
  m_modifiedVars.reserve(m_variables.size());
  assert(lowerBound.size() == upperBound.size() &&
         lowerBound.size() == m_cover.size());

  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (Int val : m_cover) {
    lb = std::min(lb, val);
    ub = std::max(ub, val);
  }

  assert(ub >= lb);

  if (closed) {
    m_lowerBound.resize(static_cast<unsigned long>(ub - lb + 1), 0);
    m_upperBound.resize(static_cast<unsigned long>(ub - lb + 1), 0);
    m_offset = lb;
  } else {
    m_lowerBound.resize(static_cast<unsigned long>(ub - lb + 3), -1);
    m_upperBound.resize(static_cast<unsigned long>(ub - lb + 3), -1);
    m_offset = lb - 1;
  }

  for (size_t i = 0; i < m_cover.size(); ++i) {
    m_lowerBound[m_cover[i] - m_offset] = lowerBound[i];
    m_upperBound[m_cover[i] - m_offset] = upperBound[i];
  }
}

void GlobalCardinality::init(Timestamp ts, Engine& e) {
  m_counts.resize(m_lowerBound.size(), SavedInt(ts, 0));

  assert(!m_id.equals(NULL_ID));

  for (size_t i = 0; i < m_variables.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_variables[i], LocalId(i));
    m_localValues.emplace_back(ts, e.getCommittedValue(m_variables[i]));
  }

  registerDefinedVariable(e, m_violationId);
}

void GlobalCardinality::recompute(Timestamp t, Engine& e) {
  for (SavedInt& c : m_counts) {
    c.setValue(t, 0);
  }

  for (size_t i = 0; i < m_variables.size(); ++i) {
    increaseCount(t, e.getValue(t, m_variables[i]));
    m_localValues[i].setValue(t, e.getValue(t, m_variables[i]));
  }

  Int excess = 0;
  Int shortage = 0;
  for (Int val : m_cover) {
    if (m_counts.at(val - m_offset).getValue(t) <
        m_lowerBound.at(val - m_offset)) {
      shortage += m_lowerBound.at(val - m_offset) -
                  m_counts.at(val - m_offset).getValue(t);
    } else if (m_upperBound.at(val - m_offset) <
               m_counts.at(val - m_offset).getValue(t)) {
      excess += m_counts.at(val - m_offset).getValue(t) -
                m_upperBound.at(val - m_offset);
    }
  }

  m_excess.setValue(t, excess);
  m_shortage.setValue(t, shortage);

  updateValue(t, e, m_violationId, std::max(excess, shortage));
}

void GlobalCardinality::notifyIntChanged(Timestamp t, Engine& e, LocalId id) {
  Int oldValue = m_localValues.at(id).getValue(t);
  Int newValue = e.getValue(t, m_variables[id]);
  if (newValue == oldValue) {
    return;
  }
  signed char dec = decreaseCount(t, oldValue);
  signed char inc = increaseCount(t, newValue);
  m_localValues.at(id).setValue(t, newValue);
  m_excess.incValue(t, (dec < 0 ? dec : 0) + (inc > 0 ? inc : 0));
  m_shortage.incValue(t, (dec > 0 ? dec : 0) + (inc < 0 ? inc : 0));

  updateValue(t, e, m_violationId,
              std::max(m_excess.getValue(t), m_shortage.getValue(t)));
}

VarId GlobalCardinality::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);

  auto index = static_cast<size_t>(m_state.getValue(t));
  if (index < m_variables.size()) {
    return m_variables.at(index);
  }
  return NULL_ID;
}

void GlobalCardinality::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  auto id = static_cast<size_t>(m_state.getValue(t));
  assert(id < m_variables.size());
  notifyIntChanged(t, e, id);
}

void GlobalCardinality::commit(Timestamp t, Engine& e) {
  Invariant::commit(t, e);

  m_excess.commitIf(t);
  m_shortage.commitIf(t);

  for (auto& localValue : m_localValues) {
    localValue.commitIf(t);
  }

  for (SavedInt& savedInt : m_counts) {
    savedInt.commitIf(t);
  }
}
