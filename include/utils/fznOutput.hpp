#pragma once

#include <string>
#include <variant>
#include <vector>

#include "invariantgraph/types.hpp"
#include "propagation/types.hpp"
#include "types.hpp"

namespace atlantis {

struct FznOutputVar {
  std::string identifier;
  std::variant<propagation::VarId, Int> var;
};

struct FznOutputVarArray {
  std::string identifier;
  std::vector<Int> indexSetSizes;
  std::vector<std::variant<propagation::VarId, Int>> vars;
  FznOutputVarArray(std::string id, std::vector<Int>&& setSizes)
      : identifier(std::move(id)), indexSetSizes(std::move(setSizes)), vars(){};
};

}  // namespace atlantis