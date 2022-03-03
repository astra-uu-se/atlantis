#pragma once

#include "core/types.hpp"
#include "fznparser/model.hpp"

#define SEARCH_VARIABLE_ARG(name, arg)                              \
  auto name = std::dynamic_pointer_cast<fznparser::SearchVariable>( \
      std::get<std::shared_ptr<fznparser::Literal>>(arg))

#define MAPPED_SEARCH_VARIABLE_ARG(name, arg, variableMap)              \
  auto name =                                                           \
      variableMap(std::dynamic_pointer_cast<fznparser::SearchVariable>( \
          std::get<std::shared_ptr<fznparser::Literal>>(arg)))

#define VALUE_ARG(name, arg) auto name = valueArgument(arg)

#define VECTOR_ARG(name, type, arg, mapper)                                    \
  std::vector<type> name;                                                      \
  do {                                                                         \
    auto literals =                                                            \
        std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(arg);       \
    name.reserve(literals.size());                                             \
    std::transform(literals.begin(), literals.end(), std::back_inserter(name), \
                   mapper);                                                    \
  } while (false)

#define VALUE_VECTOR_ARG(name, arg) auto name = valueVectorArgument(arg)

#define MAPPED_SEARCH_VARIABLE_VECTOR_ARG(name, arg, variableMap)           \
  VECTOR_ARG(                                                               \
      name, invariantgraph::VariableNode *, arg, [&](const auto &literal) { \
        return variableMap(                                                 \
            std::dynamic_pointer_cast<fznparser::SearchVariable>(literal)); \
      })

Int valueArgument(const fznparser::ConstraintArgument &argument);
std::vector<Int> valueVectorArgument(
    const fznparser::ConstraintArgument &argument);
