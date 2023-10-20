#pragma once

#include <fznparser/variables.hpp>
#include <optional>
#include <string>

const std::string& identifier(
    const std::variant<fznparser::BoolVar, fznparser::IntVar>& variable);
