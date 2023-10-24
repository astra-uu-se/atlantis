#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/constraints/allDifferent.hpp"
#include "propagation/views/equalConst.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

class AllEqualNode : public ViolationInvariantNode {
 private:
  propagation::VarId _allDifferentViolationVarId{propagation::NULL_ID};

 public:
  explicit AllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllEqualNode(std::vector<VarNodeId>&& vars, bool shouldHold = true);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_all_equal_int", 1}, {"fzn_all_equal_int_reif", 2}};
  }

  static std::unique_ptr<AllEqualNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph