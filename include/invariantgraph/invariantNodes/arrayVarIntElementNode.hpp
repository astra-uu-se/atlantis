#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/elementVar.hpp"

namespace atlantis::invariantgraph {

class ArrayVarIntElementNode : public InvariantNode {
 private:
  const Int _offset;

 public:
  ArrayVarIntElementNode(VarNodeId b, std::vector<VarNodeId>&& as,
                         VarNodeId output, Int offset);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"array_var_int_element", 3}, {"array_var_int_element_nonshifted", 3}};
  }

  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, propagation::Engine& engine) override;

  void registerNode(InvariantGraph&, propagation::Engine& engine) override;

  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace invariantgraph