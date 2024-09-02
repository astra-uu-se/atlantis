#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElementNode : public InvariantNode {
 private:
  Int _offset;

 public:
  ArrayVarElementNode(IInvariantGraph& graph, VarNodeId idx,
                      std::vector<VarNodeId>&& varVector, VarNodeId output,
                      Int offset);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId idx() const noexcept {
    return staticInputVarNodeIds().front();
  }

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
