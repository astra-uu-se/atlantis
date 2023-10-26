#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/violationInvariants/equal.hpp"
#include "propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

class IntNeNode : public ViolationInvariantNode {
 public:
  IntNeNode(VarNodeId a, VarNodeId b, VarNodeId r);

  IntNeNode(VarNodeId a, VarNodeId b, bool shouldHold);

  static std::unique_ptr<IntNeNode> fromModelConstraint(
      const fznparser::Constraint&, FznInvariantGraph&);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"int_ne", 2},
                                                       {"int_ne_reif", 3}};
  }

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph