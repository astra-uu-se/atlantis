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
  explicit GlobalCardinalityClosedNode(std::vector<VarNodeId>&& inputs,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       VarNodeId r);

  explicit GlobalCardinalityClosedNode(std::vector<VarNodeId>&& inputs,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       bool shouldHold = true);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph&) override;

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
