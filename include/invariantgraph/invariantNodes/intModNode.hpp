#pragma once

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/mod.hpp"

namespace atlantis::invariantgraph {

class IntModNode : public InvariantNode {
 public:
  IntModNode(VarNodeId a, VarNodeId b, VarNodeId output);

  ~IntModNode() override = default;

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"int_mod", 3}};
  }

  static std::unique_ptr<IntModNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVars(InvariantGraph&, propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace invariantgraph