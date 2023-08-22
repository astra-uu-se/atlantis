#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/boolOr.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class BoolOrNode : public ViolationInvariantNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  BoolOrNode(VarNodeId a, VarNodeId b, VarNodeId r)
      : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}
  BoolOrNode(VarNodeId a, VarNodeId b, bool shouldHold)
      : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                               shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"bool_or", 3}};
  }

  static std::unique_ptr<BoolOrNode> fromModelConstraint(
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