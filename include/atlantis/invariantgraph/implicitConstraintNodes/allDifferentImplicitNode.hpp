#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  explicit AllDifferentImplicitNode(InvariantGraph&,
                                    std::vector<std::shared_ptr<VarNode>>&&);

  void init() override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
