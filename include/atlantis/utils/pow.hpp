#pragma once

#include <stdexcept>

#include "atlantis/types.hpp"

namespace atlantis {

inline Int pow(Int base, Int power) {
  if (power == 0) {
    return 1;
  }
  if (power == 1) {
    return base;
  }
  if (power < 0) {
    if (base == 0) {
      throw std::runtime_error("negative power of zero");
    }
    if (base == 1) {
      return 1;
    }
    if (base == -1) {
      return power % 2 == 0 ? 1 : -1;
    }
    return 0;
  }
  Int result = 1;
  for (int i = 0; i < power; i++) {
    result *= base;
  }
  return result;
}

inline Int pow_zero_replacement(Int base, Int power, Int zeroReplacement) {
  if (power == 0) {
    return 1;
  }
  if (power == 1) {
    return base;
  }
  if (power < 0) {
    if (base == 0) {
      base = zeroReplacement;
    }
    if (base == 1) {
      return 1;
    }
    if (base == -1) {
      return power % 2 == 0 ? 1 : -1;
    }
    return 0;
  }
  Int result = 1;
  for (int i = 0; i < power; i++) {
    result *= base;
  }
  return result;
}

}  // namespace atlantis