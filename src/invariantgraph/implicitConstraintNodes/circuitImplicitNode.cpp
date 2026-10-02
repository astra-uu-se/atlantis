#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/circuitNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

CircuitImplicitNode::CircuitImplicitNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& vars,
    const Int offset)
    : ImplicitConstraintNode(graph, std::move(vars)), _offset(offset) {
  assert(InvariantNode::outputVarNodes().size() > 1);
}

void CircuitImplicitNode::init() {
  ImplicitConstraintNode::init();
  assert(std::ranges::all_of(
      outputVarNodes().begin(), outputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void CircuitImplicitNode::updateDomainTypes() {
  std::vector<Int> freeIndices;
  freeIndices.reserve(outputVarNodes().size());
  for (const auto& nId : outputVarNodes()) {
    const auto& varNode = nId;
    if (varNode->isFixed()) {
      freeIndices.emplace_back(varNode->constDomain()->lowerBound());
    }
  }

  for (size_t i = 0; i < outputVarNodes().size(); ++i) {
    const auto vId = outputVarNodes().at(i);
    auto& varNode = vId;
    assert(vId != nullptr);

    if (varNode->constDomain()->isFixed()) {
      varNode->setDomainType(DomainType::DOM_NONE);
      continue;
    }

    bool enforceDomain = false;
    for (Int val = 0; val <= static_cast<Int>(outputVarNodes().size()); ++val) {
      if (val == (static_cast<Int>(i) - 1) ||
          std::ranges::any_of(freeIndices.begin(), freeIndices.end(),
                              [&](const Int& index) { return index == val; })) {
        continue;
      }
      if (!varNode->constDomain()->contains(val)) {
        enforceDomain = true;
        break;
      }
    }
    varNode->setDomainType(enforceDomain ? DomainType::DOM_DOMAIN
                                        : DomainType::DOM_NONE);
  }
}

void CircuitImplicitNode::registerNode(propagation::SolverBase&,
                                       SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(mappingId()));

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodes().size());
  std::vector<Int> freeIndices;
  freeIndices.reserve(outputVarNodes().size());
  for (const auto& nId : outputVarNodes()) {
    const auto& varNode = nId;
    if (varNode->isFixed()) {
      freeIndices.emplace_back(varNode->constDomain()->lowerBound());
    }
  }

  for (const auto& vNode : outputVarNodes()) {
    assert(vNode != nullptr);
    searchVars.emplace_back(mapping.solverId(vNode->mappingId()), vNode->constDomain());
  }
  mapping.setNeighborhood(
      mappingId(), std::make_shared<search::neighborhoods::CircuitNeighborhood>(
                std::move(searchVars), _offset));
}

std::string CircuitImplicitNode::dotLangIdentifier() const { return "circuit"; }

}  // namespace atlantis::invariantgraph
