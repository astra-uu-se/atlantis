#pragma once
#ifdef _DEBUG
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <type_traits>

#include "atlantis/types.hpp"

namespace atlantis::overflow {

inline constexpr Int kIntMin = std::numeric_limits<Int>::min();
inline constexpr Int kIntMax = std::numeric_limits<Int>::max();

inline bool addOverflow(const Int lhs, const Int rhs, Int* result) {
#if defined(NDEBUG) && (defined(__clang__) || defined(__GNUC__))
  return __builtin_add_overflow(lhs, rhs, result);
#else
  if (rhs > 0 && lhs > kIntMax - rhs) {
    return true;
  }
  if (rhs < 0) {
    if (rhs == kIntMin && lhs < 0) {
      return true;
    }
    if (lhs < kIntMin - rhs) {
      return true;
    }
  }
  *result = lhs + rhs;
  return false;
#endif
}

inline bool subOverflow(const Int lhs, const Int rhs, Int* result) {
#if defined(NDEBUG) && (defined(__clang__) || defined(__GNUC__))
  return __builtin_sub_overflow(lhs, rhs, result);
#else
  if (rhs == kIntMin) {
    if (lhs >= 0) {
      return true;
    }
    *result = kIntMax - lhs + 1;
    return false;
  }
  return addOverflow(lhs, -rhs, result);
#endif
}

inline bool mulOverflow(const Int lhs, const Int rhs, Int* result) {
#if defined(NDEBUG) && (defined(__clang__) || defined(__GNUC__))
  return __builtin_mul_overflow(lhs, rhs, result);
#else
  if (lhs == 0 || rhs == 0) {
    *result = 0;
    return false;
  }
  if (lhs == -1) {
    if (rhs == kIntMin) {
      return true;
    }
    *result = -rhs;
    return false;
  }
  if (rhs == -1) {
    if (lhs == kIntMin) {
      return true;
    }
    *result = -lhs;
    return false;
  }
  if (lhs > 0) {
    if (rhs > 0 && lhs > kIntMax / rhs) {
      return true;
    }
    if (rhs < 0 && rhs < kIntMin / lhs) {
      return true;
    }
  } else {
    if (rhs > 0 && lhs < kIntMin / rhs) {
      return true;
    }
    if (rhs < 0 && lhs < kIntMax / rhs) {
      return true;
    }
  }
  *result = lhs * rhs;
  return false;
#endif
}

inline Int saturatingAdd(const Int lhs, const Int rhs) noexcept {
  Int result = 0;
  if (addOverflow(lhs, rhs, &result)) {
    return rhs >= 0 ? kIntMax : kIntMin;
  }
  return result;
}

inline Int saturatingSub(const Int lhs, const Int rhs) noexcept {
  Int result = 0;
  if (subOverflow(lhs, rhs, &result)) {
    return rhs < 0 ? kIntMax : kIntMin;
  }
  return result;
}

inline Int saturatingMul(const Int lhs, const Int rhs) noexcept {
  Int result = 0;
  if (mulOverflow(lhs, rhs, &result)) {
    return (lhs < 0) == (rhs < 0) ? kIntMax : kIntMin;
  }
  return result;
}

inline Int saturatingAbs(const Int value) noexcept {
  return value == kIntMin ? kIntMax : std::abs(value);
}

inline Int saturatingAbsDiff(const Int lhs, const Int rhs) noexcept {
  return saturatingAbs(saturatingSub(lhs, rhs));
}

inline size_t saturatingIntervalSize(const Int lb, const Int ub) noexcept {
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
