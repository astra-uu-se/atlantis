#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/boolEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/boolXor.hpp"

namespace invariantgraph {

class BoolEqNode : public ViolationInvariantNode {
 public:
  BoolEqNode(VarNodeId a, VarNodeId b, VarNodeId r = VarNodeId(NULL_NODE_ID));

  BoolEqNode(VarNodeId a, VarNodeId b, bool shouldHold = true);

  BoolEqNode(InvariantGraph& invariantGraph, VarNodeId a, VarNodeId b,
             bool shouldHold = true);

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"bool_eq", 2}, {"bool_eq_reif", 3}};
  }

  static std::unique_ptr<BoolEqNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace invariantgraph