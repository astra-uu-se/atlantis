#pragma once

#include <fznparser/ast.hpp>
#include <variant>

#include "core/types.hpp"
#include "utils/variant.hpp"

ObjectiveDirection getObjectiveDirection(
    const fznparser::Objective& objective) {
  return std::visit<ObjectiveDirection>(
      overloaded{
          [](const fznparser::Satisfy&) { return ObjectiveDirection::NONE; },
          [](const fznparser::Minimise&) {
            return ObjectiveDirection::MINIMIZE;
          },
          [](const fznparser::Maximise&) {
            return ObjectiveDirection::MAXIMIZE;
          }},
      objective);
}