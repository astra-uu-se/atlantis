#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/equal.hpp"
#include "constraints/notEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"

namespace invariantgraph {

class IntNeNode : public ViolationInvariantNode {
 public:
  IntNeNode(VarNodeId a, VarNodeId b, VarNodeId r)
      : ViolationInvariantNode({a, b}, r) {}
  IntNeNode(VarNodeId a, VarNodeId b, bool shouldHold)
      : ViolationInvariantNode({a, b}, shouldHold) {}

  static std::unique_ptr<IntNeNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_ne", 2},
                                                            {"int_ne_reif", 3}};
  }

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