#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class CircuitImplicitNode : public ImplicitConstraintNode {
  Int _offset;

 public:
  explicit CircuitImplicitNode(IInvariantGraph&, std::vector<VarNodeId>&&,
                               Int offset);

  void init(InvariantNodeId) override;

 protected:
  std::shared_ptr<search::neighborhoods::Neighborhood> createNeighborhood()
      override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
