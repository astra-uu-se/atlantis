#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
class VarIntCountNode : public InvariantNode {
 public:
  VarIntCountNode(InvariantGraph& graph,

                  std::vector<VarNodeId>&& vars, VarNodeId needle,
                  VarNodeId count);

  void init(const InvariantNodeId&) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] std::vector<VarNodeId> haystack() const;

  [[nodiscard]] VarNodeId needle() const;
};
}  // namespace atlantis::invariantgraph
