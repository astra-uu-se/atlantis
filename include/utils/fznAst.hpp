#pragma once

#include <fznparser/variables.hpp>
#include <optional>
#include <string_view>

const std::string_view& identifier(
    const std::variant<fznparser::BoolVar, fznparser::IntVar>& variable);
