#pragma once

#include "core/propagationEngine.hpp"
#include "core/types.hpp"

inline PropagationMode intToPropagationMode(int state) {
  switch (state) {
    case 0:
      return PropagationMode::INPUT_TO_OUTPUT;
    case 1:
    case 2:
    case 3:
      return PropagationMode::OUTPUT_TO_INPUT;
  }
}

inline OutputToInputMarkingMode intToOutputToInputMarkingMode(int state) {
  switch (state) {
    case 0:
    case 1:
      return OutputToInputMarkingMode::NONE;
    case 2:
      return OutputToInputMarkingMode::MARK_SWEEP;
    case 3:
      return OutputToInputMarkingMode::TOPOLOGICAL_SORT;
  }
}