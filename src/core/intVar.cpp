#include "core/intVar.hpp"

#include "core/types.hpp"

extern Id NULL_ID;
IntVar::IntVar() : IntVar(NULL_ID) {}
IntVar::IntVar(Id t_id)
    : IntVar(t_id, 0) {}  // todo: We need both a time-zero (when initialisation
                          // happens) but also a dummy time.
IntVar::IntVar(Id t_id, Int initValue)
    : Var(t_id),
      m_value(-1, initValue) {  // todo: We need both a time-zero (when
                                // initialisation happens) but also a dummy time.
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Initialising IntVar with id " << t_id
            << " and initial value: " << m_value.m_savedValue << "\n";
#endif
}