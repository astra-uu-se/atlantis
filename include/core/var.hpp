#pragma once
#include "types.hpp"

class Var {
  protected:
    Int m_id;

  public:
   Var(Int t_id): m_id(t_id){}
};