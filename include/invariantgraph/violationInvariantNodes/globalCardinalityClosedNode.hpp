#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "propagation/constraints/equal.hpp"
#include "propagation/constraints/globalCardinalityClosed.hpp"
#include "propagation/constraints/notEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/exists.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityClosedNode : public ViolationInvariantNode {
 private:
  const std::vector<VarNodeId> _inputs;
  const std::vector<Int> _cover;
  const std::vector<VarNodeId> _counts;
  std::vector<propagation::VarId> _intermediate{};
  std::vector<propagation::VarId> _violations{};
  propagation::VarId _shouldFailViol{propagation::NULL_ID};

 public:
  explicit GlobalCardinalityClosedNode(std::vector<VarNodeId>&& x,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       VarNodeId r);

  explicit GlobalCardinalityClosedNode(std::vector<VarNodeId>&& x,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_global_cardinality_closed", 3},
        {"fzn_global_cardinality_closed_reif", 4}};
  }

  static std::unique_ptr<GlobalCardinalityClosedNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, propagation::Engine& engine) override;

  void registerNode(InvariantGraph&, propagation::Engine& engine) override;

  [[nodiscard]] inline const std::vector<VarNodeId>& inputs() const {
    return _inputs;
  }

  [[nodiscard]] inline const std::vector<VarNodeId>& counts() const {
    return _counts;
  }
};
}  // namespace invariantgraph