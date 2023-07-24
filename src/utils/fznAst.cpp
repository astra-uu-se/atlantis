#include "utils/fznAst.hpp"

std::optional<std::string_view> definedVariable(
    const fznparser::Constraint& constraint) {
  auto annotation =
      getAnnotation<fznparser::DefinesVariableAnnotation>(constraint);
  if (!annotation) {
    return std::nullopt;
  }

  return annotation->definedVariable;
}
