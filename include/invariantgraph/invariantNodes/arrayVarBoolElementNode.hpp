#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/elementVar.hpp"

namespace invariantgraph {

class ArrayVarBoolElementNode : public InvariantNode {
 private:
  const Int _offset;

 public:
  ArrayVarBoolElementNode(VarNodeId b, std::vector<VarNodeId>&& as,
                          VarNodeId output, Int offset);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"array_var_bool_element", 3},
        {"array_var_bool_element_nonshifted", 3}};
  }

  static std::unique_ptr<ArrayVarBoolElementNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace invariantgraph