#include "utils/fznAst.hpp"

const std::string& identifier(
    const std::variant<fznparser::BoolVar, fznparser::IntVar>& variable) {
  return std::holds_alternative<fznparser::BoolVar>(variable)
             ? std::get<fznparser::BoolVar>(variable).identifier()
             : std::get<fznparser::IntVar>(variable).identifier();
}
