#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntPlusNode : public InvariantNode {
 private:
  Int _offset{0};
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  IntPlusNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
              VarNodeId output);

  void init(const InvariantNodeId&) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  bool canBeReplaced() const override;

  bool replace() override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph
