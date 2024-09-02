#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class BoolLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _offset{0};
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  BoolLinearNode(IInvariantGraph& graph,

                 std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                 VarNodeId output, Int offset = 0);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
