#pragma once

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/intDiv.hpp"

namespace atlantis::invariantgraph {

class IntDivNode : public InvariantNode {
 public:
  IntDivNode(VarNodeId a, VarNodeId b, VarNodeId output);

  ~IntDivNode() override = default;

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"int_div", 3}};
  }

  static std::unique_ptr<IntDivNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, propagation::Engine& engine) override;

  void registerNode(InvariantGraph&, propagation::Engine& engine) override;

  [[nodiscard]] VarNodeId a() const noexcept;
  [[nodiscard]] VarNodeId b() const noexcept;
};

}  // namespace invariantgraph