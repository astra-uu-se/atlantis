#pragma once

#include <limits>

template <typename T>
inline bool add_overflow(T x, T y, T& result) {
  static_assert(std::numeric_limits<T>::is_integer, "add_overflow expects integral types");
#ifdef __GNUC__
  return __builtin_add_overflow(x, y, &result);
#elifdef __clang__
  return __builtin_add_overflow(x, y, &result);
#else
  if constexpr (std::numeric_limits<T>::is_bounded) {
    if ((y > T{0} && x > std::numeric_limits<T>::max() - y) ||
      (y < T{0} && x < std::numeric_limits<T>::min() - y)) {
      return true;
    }
    result = x + y;
    return false;
  }
  return true;
#endif
}

template <typename T>
inline bool sub_overflow(T x, T y, T& result) {
  static_assert(std::numeric_limits<T>::is_integer, "sub_overflow expects integral types");
#ifdef __GNUC__
  return __builtin_sub_overflow(x, y, &result);
#elifdef __clang__
  return __builtin_sub_overflow(x, y, &result);
#else
  if constexpr (std::numeric_limits<T>::is_bounded) {
    if ((y < T{0} && x > std::numeric_limits<T>::max() + y) ||
      (y > T{0} && x < std::numeric_limits<T>::min() + y)) {
      return true;
    }
    result = x - y;
    return false;
  }
  return true;
#endif
}

template <typename T>
bool mul_overflow(T x, T y, T& result) {
  static_assert(std::numeric_limits<T>::is_integer, "mul_overflow expects integral types");
#ifdef __GNUC__
  return __builtin_mul_overflow(x, y, &result);
#elifdef __clang__
  return __builtin_mul_overflow(x, y, &result);
#else
  if constexpr (std::numeric_limits<T>::is_bounded) {
    result = x * y;
    if (x == T{0} || y == T{0}) {
      result = 0;
      return false;
    }
    result = x * y;
    return x != result / y;
  }
  return true;
#endif
}

