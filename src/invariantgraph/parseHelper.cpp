#include "parseHelper.hpp"

#include <algorithm>

static std::string_view literalTypeName(fznparser::LiteralType type) {
  switch (type) {
    case fznparser::LiteralType::PARAMETER:
      return "parameter";
    case fznparser::LiteralType::VALUE:
      return "value";
    case fznparser::LiteralType::SEARCH_VARIABLE:
      return "search variable";
    case fznparser::LiteralType::VARIABLE_ARRAY:
      return "variable array";
    case fznparser::LiteralType::PARAMETER_ARRAY:
      return "parameter array";
    default:
      return "";  // Shouldn't happen
  }
}

static inline Int valueArgument(
    const std::shared_ptr<fznparser::Literal>& literal) {
  if (literal->type() == fznparser::LiteralType::VALUE) {
    return std::dynamic_pointer_cast<fznparser::ValueLiteral>(literal)->value();
  }

  if (literal->type() == fznparser::LiteralType::PARAMETER) {
    return std::dynamic_pointer_cast<fznparser::SingleParameter>(literal)
        ->value();
  }

  throw std::logic_error(
      std::string("No value can be extracted from a literal type ")
          .append(literalTypeName(literal->type())));
}

Int valueArgument(const fznparser::ConstraintArgument& argument) {
  auto literal = std::get<std::shared_ptr<fznparser::Literal>>(argument);
  return valueArgument(literal);
}

std::vector<Int> valueVectorArgument(
    const fznparser::ConstraintArgument& argument) {
  std::vector<Int> values;

  if (std::holds_alternative<std::shared_ptr<fznparser::Literal>>(argument)) {
    auto literal = std::get<std::shared_ptr<fznparser::Literal>>(argument);

    if (literal->type() == fznparser::LiteralType::PARAMETER_ARRAY) {
      values = std::dynamic_pointer_cast<fznparser::ParameterArray>(literal)
                   ->contents();
    } else {
      throw std::logic_error(
          std::string("No value vector can be extracted from a literal type ")
              .append(literalTypeName(literal->type())));
    }
  } else {
    auto literals =
        std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(argument);

    values.reserve(literals.size());
    std::transform(literals.begin(), literals.end(), std::back_inserter(values),
                   [](const auto& literal) { return valueArgument(literal); });
  }
  return values;
}
