#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"

#include <algorithm>
#include <limits>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/search/neighborhoods/allDifferentNonUniformNeighborhood.hpp"
#include "atlantis/search/neighborhoods/allDifferentUniformNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

AllDifferentImplicitNode::AllDifferentImplicitNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputVars)
    : ImplicitConstraintNode(graph, std::move(inputVars)) {}

void AllDifferentImplicitNode::init() {
  ImplicitConstraintNode::init();
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& varNode) {
                               return varNode->isIntVar();
                             }));
}

void AllDifferentImplicitNode::updateDomainTypes() {
  if (outputVarNodes().size() <= 1) {
    return;
  }
  assert(!outputVarNodes().empty());
  assert(std::ranges::all_of(outputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& varNode) {
                               return varNode->definingNodes().size() == 1 &&
                                      varNode->outputOf() == id();
                             }));

  const auto& domain = invariantGraphConst()
                           .varNodeConst(outputVarNodes().front())
                           .constDomain();

  const bool hasSameDomain =
      std::all_of(outputVarNodes().begin() + 1, outputVarNodes().end(),
                  [&](const std::shared_ptr<VarNode>& varNode) {
                    return (*varNode->constDomain()) == (*domain);
                  });

  if (hasSameDomain && domain->size() < outputVarNodes().size()) {
    throw InconsistencyException(
        "fzn_all_different: the domain is smaller than the number of "
        "variables");
  }

  if (hasSameDomain) {
    for (const auto& nId : outputVarNodes()) {
      auto& varNode = nId;
      varNode.setDomainType(DomainType::DOM_NONE);
    }
  }
}

void AllDifferentImplicitNode::registerNode(propagation::SolverBase&,
                                            SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));

  if (outputVarNodes().size() <= 1) {
    return;
  }
  assert(!outputVarNodes().empty());
  assert(std::ranges::all_of(outputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& varNode) {
                               return varNode->definingNodes().size() == 1 &&
                                      varNode->outputOf() == id();
                             }));

  const auto& domain = invariantGraphConst()
                           .varNodeConst(outputVarNodes().front())
                           .constDomain();

  const bool hasSameDomain =
      std::all_of(outputVarNodes().begin() + 1, outputVarNodes().end(),
                  [&](const std::shared_ptr<VarNode>& varNode) {
                    return (*varNode->constDomain()) == (*domain);
                  });

  if (hasSameDomain && domain->size() < outputVarNodes().size()) {
    throw InconsistencyException(
        "fzn_all_different: she domain is smaller than the number of "
        "variables");
  }

  std::vector<search::SearchVar> searchVars;
  // "malloc(): invalid size (unsorted)" exception: don't reserve
  searchVars.reserve(outputVarNodes().size());

  if (hasSameDomain) {
    for (const auto& nId : outputVarNodes()) {
      const auto& varNode =
          nId->assert(mapping.solverId(nId) != propagation::NULL_ID);
      searchVars.emplace_back(mapping.solverId(nId), varNode.constDomain());
    }
    mapping.setNeighborhood(
        id(), std::make_shared<
                  search::neighborhoods::AllDifferentUniformNeighborhood>(
                  std::move(searchVars)));
    return;
  }
  Int domainLb = std::numeric_limits<Int>::max();
  Int domainUb = std::numeric_limits<Int>::min();
  for (const auto& varNode : outputVarNodes()) {
    const auto& varNode = varNode->searchVars.emplace_back(
        mapping.solverId(varNode), varNode.constDomain());
    domainLb = std::min<Int>(domainLb, varNode.lowerBound());
    domainUb = std::max<Int>(domainUb, varNode.upperBound());
  }
  mapping.setNeighborhood(
      id(), std::make_shared<
                search::neighborhoods::AllDifferentNonUniformNeighborhood>(
                std::move(searchVars), domainLb, domainUb));
}

std::string AllDifferentImplicitNode::dotLangIdentifier() const {
  return "all_different";
}

}  // namespace atlantis::invariantgraph
