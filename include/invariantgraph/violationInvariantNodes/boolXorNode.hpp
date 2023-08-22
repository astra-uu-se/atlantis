#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/boolEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/boolXor.hpp"

namespace invariantgraph {

class BoolXorNode : public ViolationInvariantNode {
 public:
  BoolXorNode(VarNodeId a, VarNodeId b, VarNodeId r)
      : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}
  BoolXorNode(VarNodeId a, VarNodeId b, bool shouldHold)
      : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                               shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"bool_not", 2}, {"bool_xor", 3}, {"bool_xor", 2}};
  }

  static std::unique_ptr<BoolXorNode> fromModelConstraint(
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