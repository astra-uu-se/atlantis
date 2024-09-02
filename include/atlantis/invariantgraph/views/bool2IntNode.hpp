#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class Bool2IntNode : public InvariantNode {
 public:
  Bool2IntNode(IInvariantGraph& graph, VarNodeId staticInput, VarNodeId output);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId input() const noexcept {
    return staticInputVarNodeIds().front();
  }

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
