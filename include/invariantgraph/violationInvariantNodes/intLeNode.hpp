#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "propagation/violationInvariants/lessEqual.hpp"
#include "propagation/violationInvariants/lessThan.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class IntLeNode : public ViolationInvariantNode {
 public:
  IntLeNode(VarNodeId a, VarNodeId b, VarNodeId r);

  IntLeNode(VarNodeId a, VarNodeId b, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"int_le", 2},
                                                       {"int_le_reif", 3}};
  }

  static std::unique_ptr<IntLeNode> fromModelConstraint(
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