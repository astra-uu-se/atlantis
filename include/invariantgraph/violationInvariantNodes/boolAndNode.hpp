#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/boolAnd.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class BoolAndNode : public ViolationInvariantNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  BoolAndNode(VarNodeId a, VarNodeId b, VarNodeId r)
      : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}
  BoolAndNode(VarNodeId a, VarNodeId b, bool shouldHold)
      : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                               shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"bool_and", 3}};
  }

  static std::unique_ptr<BoolAndNode> fromModelConstraint(
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