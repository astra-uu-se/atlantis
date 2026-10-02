#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class IntLinEqImplicitNode : public ImplicitConstraintNode {
  std::vector<Int> _coeffs;
  Int _offset;

 public:
  explicit IntLinEqImplicitNode(InvariantGraph&, std::vector<Int>&& coeffs,
                                std::vector<std::shared_ptr<VarNode>>&&,
                                Int offset);

  void init() override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
