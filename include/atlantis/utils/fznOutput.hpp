#pragma once

#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "atlantis/propagation/types.hpp"

namespace atlantis {

struct FznOutputVar {
  std::string identifier;
  std::variant<propagation::VarViewId, Int> var;
  FznOutputVar(std::string ident,
               const std::variant<propagation::VarViewId, Int>& var0)
      : identifier(std::move(ident)), var(var0) {}
};

struct FznOutputVarArray {
  std::string identifier;
  std::vector<Int> indexSetSizes;
  std::vector<std::variant<propagation::VarViewId, Int>> vars;
  FznOutputVarArray(std::string id, std::vector<Int>&& setSizes)
      : identifier(std::move(id)), indexSetSizes(std::move(setSizes)) {}
};

}  // namespace atlantis
