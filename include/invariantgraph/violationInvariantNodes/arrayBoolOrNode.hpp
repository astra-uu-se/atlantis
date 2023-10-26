#pragma once

#include <utility>

#include "fznparser/model.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/exists.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolOrNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolOrNode(std::vector<VarNodeId>&& as, VarNodeId output);

  ArrayBoolOrNode(std::vector<VarNodeId>&& as, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"array_bool_or", 2}};
  }

  static std::unique_ptr<ArrayBoolOrNode> fromModelConstraint(
      const fznparser::Constraint&, FznInvariantGraph&);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};

}  // namespace atlantis::invariantgraph