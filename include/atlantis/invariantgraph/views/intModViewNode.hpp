#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntModViewNode : public InvariantNode {
 private:
  Int _denominator;

 public:
  IntModViewNode(IInvariantGraph& graph, VarNodeId staticInput,
                 VarNodeId output, Int denominator);

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
