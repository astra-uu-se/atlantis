#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"

namespace invariantgraph {

class Bool2IntNode : public InvariantNode {
 public:
  Bool2IntNode(VarNodeId staticInput, VarNodeId output)
      : InvariantNode({output}, {staticInput}) {}

  ~Bool2IntNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"bool2int", 2}};
  }

  static std::unique_ptr<Bool2IntNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] VarNodeId input() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace invariantgraph