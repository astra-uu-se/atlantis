#include "atlantis/invariantgraph/implicitConstraintNodes/countImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/allDifferentNonUniformNeighborhood.hpp"
#include "atlantis/search/neighborhoods/allDifferentUniformNeighborhood.hpp"
#include "atlantis/search/neighborhoods/countNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

CountImplicitNode::CountImplicitNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputVars,
    const Int needle, const size_t amount)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _needle(needle),
      _amount(amount) {}

void CountImplicitNode::init() {
  ImplicitConstraintNode::init();
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return invariantGraphConst()
                   .varNodeConst(outputVarNodes().front())
                   .isIntVar() == vId.isIntVar();
      }));
}

void CountImplicitNode::updateDomainTypes() {
  for (const auto& nId : outputVarNodes()) {
    auto& varNode = nId;
    varNode.setDomainType(DomainType::DOM_NONE);
  }
}

void CountImplicitNode::registerNode(propagation::SolverBase&,
                                     SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));
  assert(!outputVarNodes().empty());
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return vId.definingNodes().size() == 1 && vId.outputOf() == id();
      }));

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodes().size());

  for (const auto& vId : outputVarNodes()) {
    const auto& varNode = vId;
    searchVars.emplace_back(mapping.solverId(vId), varNode.constDomain());
  }
  mapping.setNeighborhood(
      id(), std::make_shared<search::neighborhoods::CountNeighborhood>(
                std::move(searchVars), _needle, _amount));
}

std::string CountImplicitNode::dotLangIdentifier() const { return "count"; }

}  // namespace atlantis::invariantgraph
