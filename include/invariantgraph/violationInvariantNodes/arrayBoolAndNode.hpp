#pragma once

#include "fznparser/model.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/forAll.hpp"
#include "propagation/views/elementConst.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolAndNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolAndNode(std::vector<VarNodeId>&& as, VarNodeId output);

  ArrayBoolAndNode(std::vector<VarNodeId>&& as, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"array_bool_and", 2}};
  }

  static std::unique_ptr<ArrayBoolAndNode> fromModelConstraint(
      const fznparser::Constraint&, FznInvariantGraph&);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};

}  // namespace atlantis::invariantgraph