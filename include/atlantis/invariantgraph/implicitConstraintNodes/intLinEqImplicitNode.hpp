#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class IntLinEqImplicitNode : public ImplicitConstraintNode {
  std::vector<Int> _coeffs;
  Int _offset;

 public:
  explicit IntLinEqImplicitNode(IInvariantGraph&, std::vector<Int>&& coeffs,
                                std::vector<VarNodeId>&&, Int offset);

  void init(InvariantNodeId) override;

 protected:
  std::shared_ptr<search::neighborhoods::Neighborhood> createNeighborhood()
      override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
