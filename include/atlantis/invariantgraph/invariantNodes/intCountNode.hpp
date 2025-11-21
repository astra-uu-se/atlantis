#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class IntCountNode : public InvariantNode {
  Int _needle;
  Int _offset;

 public:
  IntCountNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars, Int needle,
               VarNodeId count, Int offset = 0);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<VarNodeId>& haystack() const;

  [[nodiscard]] Int needle() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
