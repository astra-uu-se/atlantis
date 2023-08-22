#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/boolLessEqual.hpp"
#include "constraints/boolLessThan.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"

namespace invariantgraph {

class BoolLtNode : public ViolationInvariantNode {
 public:
  BoolLtNode(VarNodeId a, VarNodeId b, VarNodeId r)
      : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}
  BoolLtNode(VarNodeId a, VarNodeId b, bool shouldHold)
      : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                               shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"bool_lt", 2}, {"bool_lt_reif", 3}};
  }

  static std::unique_ptr<BoolLtNode> fromModelConstraint(
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