#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/elementVar.hpp"

namespace atlantis::invariantgraph {

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

  void registerOutputVars(InvariantGraph&, propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace invariantgraph