#include "atlantis/utils/fznAst.hpp"

namespace atlantis {

const std::string& identifier(
    const std::variant<fznparser::BoolVar, fznparser::IntVar>& var) {
  return std::holds_alternative<fznparser::BoolVar>(var)
             ? std::get<fznparser::BoolVar>(var).identifier()
             : std::get<fznparser::IntVar>(var).identifier();
}

}  // namespace atlantis
