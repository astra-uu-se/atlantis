#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::invariantgraph {

class CircuitImplicitNode : public ImplicitConstraintNode {
 private:
  Int _offset;

 public:
  explicit CircuitImplicitNode(IInvariantGraph&, std::vector<VarNodeId>&&,
                               Int offset);

  void init(InvariantNodeId) override;

 protected:
  std::shared_ptr<search::neighborhoods::Neighborhood> createNeighborhood()
      override;
  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
