#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis {

struct FznOutputVar {
  std::string identifier;
  std::variant<invariantgraph::VarNodeId, Int> var;
  FznOutputVar(std::string ident,
               const std::variant<invariantgraph::VarNodeId, Int>& var0)
      : identifier(std::move(ident)), var(var0) {}
};

struct FznOutputVarArray {
  std::string identifier;
  std::vector<Int> indexSetSizes;
  std::vector<std::variant<invariantgraph::VarNodeId, Int>> vars;
  FznOutputVarArray(std::string id, std::vector<Int>&& setSizes)
      : identifier(std::move(id)), indexSetSizes(std::move(setSizes)) {}
};

class FznOutput {
  std::vector<FznOutputVar> _boolVars;
  std::vector<FznOutputVar> _intVars;
  std::vector<FznOutputVarArray> _boolVarArrays;
  std::vector<FznOutputVarArray> _intVarArrays;

public:
  FznOutput() = default;
  FznOutput(std::vector<FznOutputVar>&& boolVars,std::vector<FznOutputVar>&& intVars,std::vector<FznOutputVarArray>&& boolVarArrays,std::vector<FznOutputVarArray>&& intVarArrays);

  void appendBoolVar(FznOutputVar&&);
  void appendIntVar(FznOutputVar&&);
  void appendBoolVarArray(FznOutputVarArray&&);
  void appendIntVarArray(FznOutputVarArray&&);

  [[nodiscard]] std::vector<invariantgraph::VarNodeId> varNodeIds() const;

  void displaySolution(std::ostream&, const std::vector<Int>& solverVals) const;
};

}  // namespace atlantis
