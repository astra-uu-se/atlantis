#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class LinLeImplicitNode : public ImplicitConstraintNode {
  std::vector<Int> _coeffs;
  Int _bound;
  bool _isBinary;

 public:
  explicit LinLeImplicitNode(InvariantGraph&, std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&&, Int bound);

  void init(InvariantNodeId) override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

 protected:
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
