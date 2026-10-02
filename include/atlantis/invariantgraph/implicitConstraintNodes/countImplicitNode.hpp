#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class CountImplicitNode : public ImplicitConstraintNode {
  Int _needle;
  size_t _amount;

 public:
  explicit CountImplicitNode(InvariantGraph&,
                             std::vector<std::shared_ptr<VarNode>>&&,
                             Int needle, size_t amount);

  void init() override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
