#pragma once

#include <variant>

#include "search/cost.hpp"
#include "utils/variant.hpp"

ObjectiveDirection getObjectiveDirection(
    const fznparser::Objective& objective) {
  return std::visit<ObjectiveDirection>(
      overloaded{
          [](const fznparser::Satisfy&) { return ObjectiveDirection::NONE; },
          [](const fznparser::Minimise&) {
            return ObjectiveDirection::MINIMISE;
          },
          [](const fznparser::Maximise&) {
            return ObjectiveDirection::MAXIMISE;
          }},
      objective);
}