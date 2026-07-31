#pragma once

#include <memory>
#include <string>
#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/misc/blackboxFunction.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class BlackBoxNode : public InvariantNode {
 private:
  /// Shared rather than owned: the invariant graph is built once and then
  /// constructed into one propagation solver per search thread, so every
  /// thread's invariant refers to this same backend.
  std::shared_ptr<blackbox::BlackBoxFn> _blackBoxFn;

 public:
  explicit BlackBoxNode(InvariantGraph&,
                        std::shared_ptr<blackbox::BlackBoxFn> blackBoxFn,
                        std::vector<VarNodeId>&& intIn,
                        std::vector<VarNodeId>&& intOut);

  void init(InvariantNodeId) override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
