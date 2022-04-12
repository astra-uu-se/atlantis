#pragma once

#include <fznparser/ast.hpp>
#include <optional>

template <typename T, typename Object>
std::optional<T> getAnnotationInternal(const Object& object) {
  auto annotationIt =
      std::find_if(object.annotations.begin(), object.annotations.end(),
                   [](const auto& a) { return std::holds_alternative<T>(a); });

  if (annotationIt == object.annotations.end()) {
    return std::nullopt;
  }

  return std::get<T>(*annotationIt);
}

template <typename T>
std::optional<T> getAnnotation(const fznparser::Variable& variable) {
  return std::visit<std::optional<T>>(
      [&](const auto& var) { return getAnnotationInternal<T>(var); }, variable);
}

template <typename T>
std::optional<T> getAnnotation(const fznparser::Constraint& constraint) {
  return getAnnotationInternal<T>(constraint);
}

std::optional<fznparser::Identifier> definedVariable(
    const fznparser::Constraint& constraint);

template <typename T>
fznparser::Identifier identifier(const T& variable) {
  return std::visit<fznparser::Identifier>(
      [](const auto& var) { return var.name; }, variable);
}
