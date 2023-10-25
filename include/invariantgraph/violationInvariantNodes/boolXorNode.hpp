#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "propagation/violationInvariants/boolEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/boolXor.hpp"

namespace atlantis::invariantgraph {

class BoolXorNode : public ViolationInvariantNode {
 public:
  BoolXorNode(VarNodeId a, VarNodeId b, VarNodeId r);

  BoolXorNode(VarNodeId a, VarNodeId b, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"bool_not", 2}, {"bool_xor", 3}, {"bool_xor", 2}};
  }

  static std::unique_ptr<BoolXorNode> fromModelConstraint(
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