#include <limits>

#include "core/intVar.hpp"
#include "core/types.hpp"

extern Id NULL_ID;
IntVar::IntVar() : IntVar(NULL_ID) {}
IntVar::IntVar(Id t_id)
    : IntVar(t_id, 0) {}

IntVar::IntVar(Id t_id, Int initValue)
    : IntVar(t_id, initValue, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max()) {}

IntVar::IntVar(Id t_id, Int initValue, Int t_lowerBound, Int t_upperBound)
    : Var(t_id),
      m_value(-1, initValue, t_lowerBound, t_upperBound) // todo: We need both a time-zero (when
                                                         // initialisation happens) but also a dummy time.
      {
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Initialising IntVar with id " << t_id
            << " and initial value: " << m_value.m_savedValue << "\n";
#endif
}