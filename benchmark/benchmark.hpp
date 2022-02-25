#pragma once

#include "core/propagationEngine.hpp"
#include "core/types.hpp"

inline PropagationMode intToPropagationMode(int state) {
  switch (state) {
    case 3:
    case 2:
    case 1:
      return PropagationMode::OUTPUT_TO_INPUT;
    case 0:
    default:
      return PropagationMode::INPUT_TO_OUTPUT;
  }
}

inline OutputToInputMarkingMode intToOutputToInputMarkingMode(int state) {
  switch (state) {
    case 3:
      return OutputToInputMarkingMode::TOPOLOGICAL_SORT;
    case 2:
      return OutputToInputMarkingMode::MARK_SWEEP;
    case 1:
    case 0:
    default:
      return OutputToInputMarkingMode::NONE;
  }
}