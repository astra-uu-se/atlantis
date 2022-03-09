#pragma once

#include <algorithm>

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

#define VECTOR_ARG(name, type, arg, mapper) \
  std::vector<type> name;                   \
  do {                                      \
    variableArray(arg, mapper, name);       \
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

template <typename F, typename T = std::invoke_result_t<F>>
inline void variableArray(const fznparser::ConstraintArgument &argument,
                          F mapper, std::vector<T> &result) {
  if (std::holds_alternative<std::vector<std::shared_ptr<fznparser::Literal>>>(
          argument)) {
    auto literals =
        std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(argument);
    result.reserve(literals.size());
    std::transform(literals.begin(), literals.end(), std::back_inserter(result),
                   mapper);
  } else {
    auto literal = std::get<std::shared_ptr<fznparser::Literal>>(argument);
    auto varArray =
        std::dynamic_pointer_cast<fznparser::VariableArray>(literal);

    result.reserve(varArray->contents().size());
    std::transform(varArray->contents().begin(), varArray->contents().end(),
                   std::back_inserter(result), mapper);
  }
}
