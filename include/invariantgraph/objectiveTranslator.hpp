#pragma once

#include <variant>

#include "search/cost.hpp"
#include "utils/variant.hpp"

propagation::ObjectiveDirection getpropgation::ObjectiveDirection(
    const fznparser::Objective& objective) {
  return std::visit<propagation::ObjectiveDirection>(
      overloaded{[](const fznparser::Satisfy&) {
                   return propagation::ObjectiveDirection::NONE;
                 },
                 [](const fznparser::Minimise&) {
                   return propagation::ObjectiveDirection::MINIMIZE;
                 },
                 [](const fznparser::Maximise&) {
                   return propagation::ObjectiveDirection::MAXIMIZE;
                 }},
      objective);
}