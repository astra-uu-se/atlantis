#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  explicit AllDifferentImplicitNode(InvariantGraph&, std::vector<VarNodeId>&&);

  void init(InvariantNodeId) override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

 protected:
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
