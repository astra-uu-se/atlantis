#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"

#include <numeric>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"
#include "atlantis/search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::invariantgraph {

AllDifferentImplicitNode::AllDifferentImplicitNode(
    std::vector<VarNodeId>&& inputVars)
    : ImplicitConstraintNode(std::move(inputVars)) {}

void AllDifferentImplicitNode::init(InvariantGraph& graph,
                                    const InvariantNodeId& id) {
  ImplicitConstraintNode::init(graph, id);
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
AllDifferentImplicitNode::createNeighbourhood(InvariantGraph& graph,
                                              propagation::SolverBase& solver) {
  if (outputVarNodeIds().size() <= 1) {
    return nullptr;
  }
  bool hasSameDomain = true;
  assert(!outputVarNodeIds().empty());

  const auto& domain =
      graph.varNodeConst(outputVarNodeIds().front()).constDomain();

  for (size_t i = 1; i < outputVarNodeIds().size(); ++i) {
    if (graph.varNodeConst(outputVarNodeIds().at(i)).constDomain() != domain) {
      hasSameDomain = false;
      break;
    }
  }

  std::vector<search::SearchVar> searchVars;
  // "malloc(): invalid size (unsorted)" exception: don't reserve
  searchVars.reserve(outputVarNodeIds().size());

  if (hasSameDomain) {
    for (const auto& nId : outputVarNodeIds()) {
      auto& varNode = graph.varNode(nId);
      assert(varNode.varId() != propagation::NULL_ID);
      searchVars.emplace_back(varNode.varId(),
                              SearchDomain{varNode.constDomain().lowerBound(),
                                           varNode.constDomain().upperBound()});
      varNode.setDomainType(VarNode::DomainType::NONE);
    }
    const auto& vals =
        graph.varNode(outputVarNodeIds().front()).domain().values();
    std::vector<Int> domainValues;
    domainValues.reserve(vals.size());
    std::copy(vals.begin(), vals.end(), std::back_inserter(domainValues));

    return std::make_shared<
        search::neighbourhoods::AllDifferentUniformNeighbourhood>(
        std::move(searchVars), std::move(domainValues));
  } else {
    Int domainLb = std::numeric_limits<Int>::max();
    Int domainUb = std::numeric_limits<Int>::min();
    for (const auto& nId : outputVarNodeIds()) {
      auto& varNode = graph.varNode(nId);
      searchVars.emplace_back(varNode.varId(),
                              SearchDomain{varNode.constDomain().lowerBound(),
                                           varNode.constDomain().upperBound()});
      domainLb = std::min<Int>(domainLb, varNode.constDomain().lowerBound());
      domainUb = std::max<Int>(domainUb, varNode.constDomain().upperBound());
    }
    return std::make_shared<
        search::neighbourhoods::AllDifferentNonUniformNeighbourhood>(
        std::move(std::move(searchVars)), domainLb, domainUb, solver);
  }
}

std::string AllDifferentImplicitNode::dotLangIdentifier() const {
  return "all_different";
}

}  // namespace atlantis::invariantgraph
