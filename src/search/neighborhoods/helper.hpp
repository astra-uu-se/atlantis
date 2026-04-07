#pragma once
#include <sys/cdefs.h>

#include "atlantis/types.hpp"

namespace atlantis::search {
template<bool Violation>
__always_inline
Int toInt(const bool b) {
  if constexpr(Violation) {
    return b ? 0 : 1;
  }  else {
    return b ? 1 : 0;
  }
}

template <bool Violation>
__always_inline
bool toBool(const Int val) {
  if constexpr (Violation) {
    return val == 0;
  } else {
    return val == 1;
  }
}

}