#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

class TableImplicitNode : public ImplicitConstraintNode {
  std::vector<std::vector<Int>> _table;
 public:
  explicit TableImplicitNode(InvariantGraph&, std::vector<VarNodeId>&&, std::vector<std::vector<Int>>&&);

  void init(InvariantNodeId) override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

 protected:
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
