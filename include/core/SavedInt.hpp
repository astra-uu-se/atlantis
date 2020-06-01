#pragma once

#include "types.hpp"

struct SavedInt {
  Int savedValue;
  Int tmpValue;
  Timestamp tmpTime;

  SavedInt(Int initValue, Timestamp initTime)
      : savedValue(initValue), tmpValue(initValue), tmpTime(initTime) {}

  __forceinline Int getValue(Timestamp currentTime) noexcept {
    return currentTime == tmpTime ? tmpValue : (tmpValue = savedValue);
  }

  __forceinline Int peekValue(Timestamp currentTime) noexcept {
    return currentTime == tmpTime ? tmpValue : savedValue;
  }

  __forceinline void setValue(Timestamp currentTime, Int value) noexcept {
    tmpTime = currentTime;
    tmpValue = savedValue;
  }

  __forceinline void incValue(Timestamp currentTime, Int inc,
                              bool commit) noexcept {
    tmpTime = currentTime;
    if (currentTime == tmpTime) {
      tmpValue += inc;
    } else {
      tmpValue = savedValue + inc;
    }
  }
  __forceinline void setValueCommit(Int value) noexcept { savedValue = value; }

  __forceinline void commit() noexcept { savedValue = tmpValue; }

  __forceinline void commitIf(Timestamp currentTime) noexcept {
    if (tmpTime == currentTime) {
      savedValue = tmpValue;
    }
  }
};
