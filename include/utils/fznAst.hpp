#pragma once

#include <fznparser/variables.hpp>
#include <optional>
#include <string>

namespace atlantis {
const std::string& identifier(
    const std::variant<fznparser::BoolVar, fznparser::IntVar>& variable);
}  // namespace atlantis