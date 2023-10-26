#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/violationInvariants/boolLessEqual.hpp"
#include "propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {

class BoolLeNode : public ViolationInvariantNode {
 public:
  BoolLeNode(VarNodeId a, VarNodeId b, VarNodeId r);

  BoolLeNode(VarNodeId a, VarNodeId b, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"bool_le", 2},
                                                       {"bool_le_reif", 3}};
  }

  static std::unique_ptr<BoolLeNode> fromModelConstraint(
      const fznparser::Constraint&, FznInvariantGraph&);

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