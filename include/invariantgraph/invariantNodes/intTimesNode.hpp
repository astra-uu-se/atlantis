#pragma once

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/times.hpp"

namespace invariantgraph {

class IntTimesNode : public InvariantNode {
 public:
  IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output)
      : InvariantNode({output}, {a, b}) {}

  ~IntTimesNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_times", 3}};
  }

  static std::unique_ptr<IntTimesNode> fromModelConstraint(
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