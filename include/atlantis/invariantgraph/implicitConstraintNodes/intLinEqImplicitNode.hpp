#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::invariantgraph {

class IntLinEqImplicitNode : public ImplicitConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _offset;

 public:
  explicit IntLinEqImplicitNode(std::vector<Int>&& coeffs,
                                std::vector<VarNodeId>&&, Int offset);

  void init(InvariantGraph&, const InvariantNodeId&) override;

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      InvariantGraph&, propagation::SolverBase&) override;

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
