#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  explicit AllDifferentImplicitNode(IInvariantGraph&, std::vector<VarNodeId>&&);

  void init(InvariantNodeId) override;

 protected:
  std::shared_ptr<search::neighborhoods::Neighborhood> createNeighborhood()
      override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
