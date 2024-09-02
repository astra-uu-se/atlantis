#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class IntLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _offset;
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  IntLinearNode(IInvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& vars, VarNodeId output,
                Int offset = 0);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
