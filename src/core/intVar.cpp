#include "core/intVar.hpp"

IntVar::IntVar(Int t_lowerBound, Int t_upperBound)
    : IntVar(NULL_ID, t_lowerBound, t_upperBound) {}

IntVar::IntVar(VarId t_id, Int t_lowerBound, Int t_upperBound)
    : IntVar(t_id, 0, t_lowerBound, t_upperBound) {}

IntVar::IntVar(VarId t_id, Int initValue, Int t_lowerBound, Int t_upperBound)
    : IntVar(NULL_TIMESTAMP, t_id, initValue, t_lowerBound, t_upperBound) {}

IntVar::IntVar(Timestamp t, VarId t_id, Int initValue, Int t_lowerBound,
               Int t_upperBound)
    : Var(t_id),
      m_value(t, initValue),  // todo: We need both a time-zero (when
                              // initialisation happens) but also a dummy time.
      m_lowerBound(t_lowerBound),
      m_upperBound(t_upperBound) {
  if (t_lowerBound > t_upperBound) {
    throw std::out_of_range(
        "Lower bound must be smaller than or equal to upper bound");
  }
}

std::ostream& operator<<(std::ostream& out, IntVar const& var) {
  out << "IntVar(id: " << var.m_id.id;
  out << ",c: " << var.m_value.getCommittedValue();
  out << ",t: " << var.m_value.getValue(var.m_value.getTmpTimestamp());
  out << ",ts: " << var.m_value.getTmpTimestamp();
  out << ")";
  return out;
}