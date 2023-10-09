#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "constraints/lessThan.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"

namespace invariantgraph {

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