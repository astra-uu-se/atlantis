#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityClosedNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _cover;

 public:
  explicit GlobalCardinalityClosedNode(InvariantGraph& graph,

                                       std::vector<VarNodeId>&& inputs,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       VarNodeId r);

  explicit GlobalCardinalityClosedNode(InvariantGraph& graph,

                                       std::vector<VarNodeId>&& inputs,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       bool shouldHold = true);

  void init(const InvariantNodeId&) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;
};

}  // namespace atlantis::invariantgraph
