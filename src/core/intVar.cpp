#include "core/intVar.hpp"

#include "core/engine.hpp"

IntVar::IntVar() : IntVar(Engine::NULL_ID) {}
IntVar::IntVar(Id t_id) : Var(t_id), m_value(0, 0) {
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Initialising IntVar with id " << t_id << "\n";
#endif
}