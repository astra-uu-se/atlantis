#include "atlantis/invariantgraph/implicitConstraintNodes/tableImplicitNode.hpp"

#include <algorithm>
#include <limits>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/search/neighborhoods/tableNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

TableImplicitNode::TableImplicitNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputVars,
    std::vector<std::vector<Int>>&& table)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _table(std::move(table)) {}

void TableImplicitNode::init() { ImplicitConstraintNode::init(); }

void TableImplicitNode::updateDomainTypes() {
  if (outputVarNodes().size() <= 1) {
    return;
  }
  assert(!outputVarNodes().empty());
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
        return vNode->definingNodes().size() == 1 && vNode->outputOf().get() == this;
      }));

  for (const auto& nId : outputVarNodes()) {
    nId->setDomainType(DomainType::DOM_NONE);
  }
}

void TableImplicitNode::registerNode(propagation::SolverBase&,
                                     SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(ptrConst()));

  if (outputVarNodes().size() <= 1) {
    return;
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
        return vNode->definingNodes().size() == 1 && vNode->outputOf().get() == this;
      }));

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodes().size());

  for (const auto& varNode : outputVarNodes()) {
    assert(mapping.solverId(varNode) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(varNode), varNode->constDomain());
  }
  mapping.setNeighborhood(
      ptrConst(), std::make_shared<search::neighborhoods::TableNeighborhood>(
                std::move(searchVars), std::vector<std::vector<Int>>{_table}));
}

std::string TableImplicitNode::dotLangIdentifier() const { return "table"; }

}  // namespace atlantis::invariantgraph
