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
    InvariantGraph& graph, std::vector<VarNodeId>&& inputVars)
    : ImplicitConstraintNode(graph, std::move(inputVars)) {}

void AllDifferentImplicitNode::init(InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void AllDifferentImplicitNode::updateDomainTypes() {
  if (outputVarNodeIds().size() <= 1) {
    return;
  }
  assert(!outputVarNodeIds().empty());
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return invariantGraphConst().varNodeConst(vId).definingNodes().size() ==
               1 &&
           invariantGraphConst().varNodeConst(vId).outputOf() == id();
  }));

  const auto& domain = invariantGraphConst()
                           .varNodeConst(outputVarNodeIds().front())
                           .constDomain();

  const bool hasSameDomain = std::all_of(
      outputVarNodeIds().begin() + 1, outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return (*invariantGraphConst().varNodeConst(vId).constDomain()) ==
               (*domain);
      });

  if (hasSameDomain && domain->size() < outputVarNodeIds().size()) {
    throw InconsistencyException(
        "fzn_all_different: the domain is smaller than the number of "
        "variables");
  }

  if (hasSameDomain) {
    for (const auto& nId : outputVarNodeIds()) {
      auto& varNode = invariantGraph().varNode(nId);
      varNode.setDomainType(DomainType::DOM_NONE);
    }
  }
}

void AllDifferentImplicitNode::registerNode(propagation::SolverBase&,
                                            SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));

  if (outputVarNodeIds().size() <= 1) {
    return;
  }
  assert(!outputVarNodeIds().empty());
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return invariantGraphConst().varNodeConst(vId).definingNodes().size() ==
               1 &&
           invariantGraphConst().varNodeConst(vId).outputOf() == id();
  }));

  const auto& domain = invariantGraphConst()
                           .varNodeConst(outputVarNodeIds().front())
                           .constDomain();

  const bool hasSameDomain = std::all_of(
      outputVarNodeIds().begin() + 1, outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return (*invariantGraphConst().varNodeConst(vId).constDomain()) ==
               (*domain);
      });

  if (hasSameDomain && domain->size() < outputVarNodeIds().size()) {
    throw InconsistencyException(
        "fzn_all_different: she domain is smaller than the number of "
        "variables");
  }

  std::vector<search::SearchVar> searchVars;
  // "malloc(): invalid size (unsorted)" exception: don't reserve
  searchVars.reserve(outputVarNodeIds().size());

  if (hasSameDomain) {
    for (const auto& nId : outputVarNodeIds()) {
      const auto& varNode = invariantGraphConst().varNodeConst(nId);
      assert(mapping.solverId(nId) != propagation::NULL_ID);
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
  for (const auto& vId : outputVarNodeIds()) {
    const auto& varNode = invariantGraphConst().varNodeConst(vId);
    searchVars.emplace_back(mapping.solverId(vId), varNode.constDomain());
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
