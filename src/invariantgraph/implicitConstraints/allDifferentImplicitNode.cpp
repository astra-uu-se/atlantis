#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

AllDifferentImplicitNode::AllDifferentImplicitNode(
    std::vector<VarNodeId>&& inputVars)
    : ImplicitConstraintNode(std::move(inputVars)) {}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
AllDifferentImplicitNode::createNeighbourhood(
    propagation::SolverBase& solver, std::vector<search::SearchVar>&& vars) {
  if (vars.size() <= 1) {
    return nullptr;
  }
  bool hasSameDomain = true;
  assert(!vars.empty());

  auto& domain = vars.front().domain();
  for (auto& var : vars) {
    if (var.domain() != domain) {
      hasSameDomain = false;
      break;
    }
  }
  if (hasSameDomain) {
    return std::make_shared<
        search::neighbourhoods::AllDifferentUniformNeighbourhood>(
        std::move(vars), std::vector<Int>(domain.values()), solver);
  } else {
    Int domainLb = std::numeric_limits<Int>::max();
    Int domainUb = std::numeric_limits<Int>::min();
    for (auto& var : vars) {
      domainLb = std::min<Int>(domainLb, var.domain().lowerBound());
      domainUb = std::max<Int>(domainUb, var.domain().upperBound());
    }
    return std::make_shared<
        search::neighbourhoods::AllDifferentNonUniformNeighbourhood>(
        std::move(vars), domainLb, domainUb, solver);
  }
}

}  // namespace atlantis::invariantgraph
