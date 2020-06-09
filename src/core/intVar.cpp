#include "core/intVar.hpp"

#include "core/types.hpp"

extern Id NULL_ID;
IntVar::IntVar() : IntVar(NULL_ID) {}
IntVar::IntVar(Id t_id) : Var(t_id), m_value(-1, 0) { // todo: We need both a time-zero (when initalisation happens) but also a dummy time.
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Initialising IntVar with id " << t_id << "\n";
#endif
}