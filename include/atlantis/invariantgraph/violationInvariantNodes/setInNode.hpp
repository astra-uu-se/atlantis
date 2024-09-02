#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class SetInNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _values;
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit SetInNode(VarNodeId input, std::vector<Int>&& values, VarNodeId r);

  explicit SetInNode(VarNodeId input, std::vector<Int>&& values,
                     bool shouldHold = true);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
