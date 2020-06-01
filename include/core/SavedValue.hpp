#pragma once

#include "types.hpp"

template<typename T>
struct SavedValue {
  T savedValue;
  T tmpValue;
  Timestamp tmpTime;

  SavedValue(T initValue, Timestamp initTime)
      : savedValue(initValue), tmpValue(initValue), tmpTime(initTime) {}

  __forceinline T getValue(Timestamp currentTime) noexcept {
    return currentTime == tmpTime ? tmpValue : (tmpValue = savedValue);
  }

  __forceinline T peekValue(Timestamp currentTime) noexcept {
    return currentTime == tmpTime ? tmpValue : savedValue;
  }

  __forceinline void setValue(Timestamp currentTime, T value) noexcept {
    tmpTime = currentTime;
    tmpValue = savedValue;
  }
  __forceinline void setValueCommit(T value) noexcept { savedValue = value; }

  __forceinline void commit() noexcept { savedValue = tmpValue; }

  __forceinline void commitIf(Timestamp currentTime) noexcept {
    if (tmpTime == currentTime) {
      savedValue = tmpValue;
    }
  }
};
