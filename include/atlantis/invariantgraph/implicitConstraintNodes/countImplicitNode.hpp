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
  explicit CountImplicitNode(InvariantGraph&, std::vector<VarNodeId>&&,
                             Int needle, size_t amount);

  void init(InvariantNodeId) override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

 protected:
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
