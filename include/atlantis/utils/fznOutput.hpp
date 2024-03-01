#pragma once

#include <string>
#include <variant>
#include <vector>

#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis {

struct FznOutputVar {
  std::string identifier;
  std::variant<propagation::VarId, Int> var;
  FznOutputVar(std::string ident, std::variant<propagation::VarId, Int> var0)
      : identifier(ident), var(var0) {}
};

struct FznOutputVarArray {
  std::string identifier;
  std::vector<Int> indexSetSizes;
  std::vector<std::variant<propagation::VarId, Int>> vars;
  FznOutputVarArray(std::string id, std::vector<Int>&& setSizes)
      : identifier(std::move(id)), indexSetSizes(std::move(setSizes)), vars(){};
};

}  // namespace atlantis
