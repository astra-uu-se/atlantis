#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/constraints/allDifferent.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllDifferentNode(std::vector<VarNodeId>&& vars, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_all_different_int", 1}, {"fzn_all_different_int_reif", 2}};
  }

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  bool prune(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph