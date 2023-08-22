#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/elementVar.hpp"

namespace invariantgraph {

class ArrayVarIntElementNode : public InvariantNode {
 private:
  const Int _offset;

 public:
  ArrayVarIntElementNode(VarNodeId b, std::vector<VarNodeId>&& as,
                         VarNodeId output, Int offset)
      : InvariantNode({output}, {b}, std::move(as)), _offset(offset) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_var_int_element", 3}, {"array_var_int_element_nonshifted", 3}};
  }

  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace invariantgraph