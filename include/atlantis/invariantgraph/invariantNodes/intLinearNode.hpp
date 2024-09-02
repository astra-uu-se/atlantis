#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class IntLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _offset;
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  IntLinearNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                VarNodeId output, Int offset = 0);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void updateState(InvariantGraph&) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph&) override;

  [[nodiscard]] bool canBeMadeImplicit(const InvariantGraph&) const override;

  [[nodiscard]] bool makeImplicit(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
