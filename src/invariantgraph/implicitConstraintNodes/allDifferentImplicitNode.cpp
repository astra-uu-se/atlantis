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
                                      varNode->outputOf().get() == this;
                             }));

  const auto& domain = outputVarNode(0)
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
    for (const auto& varNode : outputVarNodes()) {
      varNode->setDomainType(DomainType::DOM_NONE);
    }
  }
}

void AllDifferentImplicitNode::registerNode(propagation::SolverBase&,
                                            SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(TODO));

  if (outputVarNodes().size() <= 1) {
    return;
  }
  assert(!outputVarNodes().empty());
  assert(std::ranges::all_of(outputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& varNode) {
                               return varNode->definingNodes().size() == 1 &&
                                      varNode->outputOf().get() == this;
                             }));

  const auto& domain = outputVarNode(0)
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
    for (const auto& vNode : outputVarNodes()) {
      assert(mapping.solverId(vNode->mappingId()) != propagation::NULL_ID);
      searchVars.emplace_back(mapping.solverId(vNode->mappingId()), vNode->constDomain());
    }
    mapping.setNeighborhood(
        TODO, std::make_shared<
                  search::neighborhoods::AllDifferentUniformNeighborhood>(
                  std::move(searchVars)));
    return;
  }
  Int domainLb = std::numeric_limits<Int>::max();
  Int domainUb = std::numeric_limits<Int>::min();
  for (const auto& varNode : outputVarNodes()) {
    searchVars.emplace_back(
        mapping.solverId(varNode->mappingId()), varNode->constDomain());
    domainLb = std::min<Int>(domainLb, varNode->lowerBound());
    domainUb = std::max<Int>(domainUb, varNode->upperBound());
  }
  mapping.setNeighborhood(
      TODO, std::make_shared<
                search::neighborhoods::AllDifferentNonUniformNeighborhood>(
                std::move(searchVars), domainLb, domainUb));
}

std::string AllDifferentImplicitNode::dotLangIdentifier() const {
  return "all_different";
}

}  // namespace atlantis::invariantgraph
