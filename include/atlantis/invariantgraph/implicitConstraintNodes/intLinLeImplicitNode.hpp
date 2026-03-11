#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class IntLinLeImplicitNode : public ImplicitConstraintNode {
  std::vector<Int> _coeffs;
  Int _offset;

 public:
  explicit IntLinLeImplicitNode(InvariantGraph&, std::vector<Int>&& coeffs,
                                std::vector<VarNodeId>&&, Int offset);

  void init(InvariantNodeId) override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

 protected:
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
