#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

class Bool2IntNode : public InvariantNode {
 public:
  Bool2IntNode(VarNodeId staticInput, VarNodeId output);

  ~Bool2IntNode() override = default;

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"bool2int", 2}};
  }

  static std::unique_ptr<Bool2IntNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId input() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace atlantis::invariantgraph