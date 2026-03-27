#pragma once

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <type_traits>

#include "atlantis/types.hpp"

namespace atlantis::overflow {

inline constexpr Int kIntMin = std::numeric_limits<Int>::min();
inline constexpr Int kIntMax = std::numeric_limits<Int>::max();

inline Int saturatingAdd(Int lhs, Int rhs) noexcept {
  Int result = 0;
#if defined(__clang__) || defined(__GNUC__)
  if (__builtin_add_overflow(lhs, rhs, &result)) {
    return rhs >= 0 ? kIntMax : kIntMin;
  }
  return result;
#else
  if (rhs > 0 && lhs > kIntMax - rhs) {
    return kIntMax;
  }
  if (rhs < 0) {
    if (rhs == kIntMin) {
      return lhs < 0 ? kIntMin : lhs + rhs;
    }
    if (lhs < kIntMin - rhs) {
      return kIntMin;
    }
  }
  return lhs + rhs;
#endif
}

inline Int saturatingSub(Int lhs, Int rhs) noexcept {
  Int result = 0;
#if defined(__clang__) || defined(__GNUC__)
  if (__builtin_sub_overflow(lhs, rhs, &result)) {
    return rhs < 0 ? kIntMax : kIntMin;
  }
  return result;
#else
  if (rhs == kIntMin) {
    return lhs >= 0 ? kIntMax : kIntMax + lhs + 1;
  }
  return saturatingAdd(lhs, -rhs);
#endif
}

inline Int saturatingMul(Int lhs, Int rhs) noexcept {
  Int result = 0;
#if defined(__clang__) || defined(__GNUC__)
  if (__builtin_mul_overflow(lhs, rhs, &result)) {
    return (lhs < 0) == (rhs < 0) ? kIntMax : kIntMin;
  }
  return result;
#else
  if (lhs == 0 || rhs == 0) {
    return 0;
  }
  if (lhs == -1) {
    return rhs == kIntMin ? kIntMax : -rhs;
  }
  if (rhs == -1) {
    return lhs == kIntMin ? kIntMax : -lhs;
  }
  if (lhs > 0) {
    if (rhs > 0 && lhs > kIntMax / rhs) {
      return kIntMax;
    }
    if (rhs < 0 && rhs < kIntMin / lhs) {
      return kIntMin;
    }
  } else {
    if (rhs > 0 && lhs < kIntMin / rhs) {
      return kIntMin;
    }
    if (rhs < 0 && lhs < kIntMax / rhs) {
      return kIntMax;
    }
  }
  return lhs * rhs;
#endif
}

inline Int saturatingAbs(Int value) noexcept {
  return value == kIntMin ? kIntMax : std::abs(value);
}

inline Int saturatingAbsDiff(Int lhs, Int rhs) noexcept {
  return saturatingAbs(saturatingSub(lhs, rhs));
}

inline size_t saturatingIntervalSize(Int lb, Int ub) noexcept {
  if (ub < lb) {
    return 0;
  }
  using UInt = std::make_unsigned_t<Int>;
  const UInt width = static_cast<UInt>(ub) - static_cast<UInt>(lb);
  constexpr UInt kSizeMax =
      static_cast<UInt>(std::numeric_limits<size_t>::max());
  if (width >= kSizeMax) {
    return std::numeric_limits<size_t>::max();
  }
  return static_cast<size_t>(width + UInt{1});
}

}  // namespace atlantis::overflow
