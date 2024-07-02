#include "atlantis/invariantgraph/violationInvariantNodes/circuitNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/invariantgraph/views/intModViewNode.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

namespace atlantis::invariantgraph {

CircuitNode::CircuitNode(std::vector<VarNodeId>&& vars)
    : ViolationInvariantNode(std::move(vars), true) {}

bool CircuitNode::canBeMadeImplicit(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         shouldHold() &&
         std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& nId) {
                       return graph.varNodeConst(nId).isFixed() ||
                              graph.varNodeConst(nId).definingNodes().empty();
                     });
}

bool CircuitNode::makeImplicit(InvariantGraph& graph) {
  if (!canBeMadeImplicit(graph)) {
    return false;
  }
  graph.addImplicitConstraintNode(std::make_unique<CircuitImplicitNode>(
      std::vector<VarNodeId>{staticInputVarNodeIds()}));
  return true;
}

bool CircuitNode::canBeReplaced(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE && !canBeMadeImplicit(graph);
}

bool CircuitNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  /*
  predicate mod_circuit(array[int] of var int: x) =
  if length(x) = 0 then
    true
  else
    let { set of int: S = index_set(x),
          int: l = min(S),
          int: n = card(S),
          array[S] of var 0..n+1: order,
          array[S] of var 0..n+1: offset,
          array[S] of var 0..n+1: modulo
        } in
    all_different(x) /\
    all_different(order) /\
    forall(i in S)(x[i] != i) /\
    order[l] = 1 /\
    forall(i in S)(offset[i] = order[i] + 1) /\
    forall(i in S)(modulo[i] = offset[i] mod n) /\
    forall(i in S)(
      let {
        array[S] of var 0..n+1: sansI = array1d(
        S,
        [if i != j then order[j] else 1 endif | j in S])
      } in sansI[x[i]] = modulo[i]) endif;
  */
  graph.addInvariantNode(std::make_unique<AllDifferentNode>(
      std::vector<VarNodeId>{staticInputVarNodeIds()}, true));

  std::vector<VarNodeId> orderVars;
  orderVars.reserve(staticInputVarNodeIds().size());
  std::vector<VarNodeId> offsetVars;
  offsetVars.reserve(staticInputVarNodeIds().size());
  std::vector<VarNodeId> modoluVars;
  modoluVars.reserve(staticInputVarNodeIds().size());
  // order[l] == 1:
  orderVars.emplace_back(graph.retrieveIntVarNode(1));
  // order[l] == 1 -> offset[0] = 2
  offsetVars.emplace_back(graph.retrieveIntVarNode(2));
  // order[l] == 1 -> modulo[0] = 2
  modoluVars.emplace_back(graph.retrieveIntVarNode(2));

  for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
    orderVars.emplace_back(graph.retrieveIntVarNode(
        SearchDomain{1, static_cast<Int>(staticInputVarNodeIds().size())},
        VarNode::DomainType::NONE));
    offsetVars.emplace_back(graph.retrieveIntVarNode(
        SearchDomain{1, static_cast<Int>(staticInputVarNodeIds().size()) + 1},
        VarNode::DomainType::NONE));
    modoluVars.emplace_back(graph.retrieveIntVarNode(
        SearchDomain{0, static_cast<Int>(staticInputVarNodeIds().size()) - 1},
        VarNode::DomainType::NONE));
    // offset[i] = order[i] + 1
    graph.addInvariantNode(std::make_unique<IntScalarNode>(
        offsetVars.at(i), orderVars.at(i), 1, 1));
    // modulo[i] = offset[i] mod n
    graph.addInvariantNode(std::make_unique<IntModViewNode>(
        modoluVars.at(i), offsetVars.at(i), staticInputVarNodeIds().size()));
  }
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    std::vector<VarNodeId> orderSansI;
    orderSansI.reserve(staticInputVarNodeIds().size());
    for (size_t j = 0; j < staticInputVarNodeIds().size(); ++j) {
      if (i == j) {
        // remove self cycle:
        orderSansI.emplace_back(graph.retrieveIntVarNode(1));
      } else {
        orderSansI.emplace_back(orderVars.at(j));
      }
    }
    // sansI[x[i]] = modulo[i]
    graph.addInvariantNode(std::make_unique<ArrayVarElementNode>(
        staticInputVarNodeIds().at(i), std::move(orderSansI), modoluVars.at(i),
        1));
  }

  graph.addInvariantNode(
      std::make_unique<AllDifferentNode>(std::move(orderVars), true));

  return true;
}

void CircuitNode::registerOutputVars(InvariantGraph&,
                                     propagation::SolverBase&) {
  throw std::runtime_error("CircuitNode::registerOutputVars not implemented");
}

void CircuitNode::registerNode(InvariantGraph&, propagation::SolverBase&) {
  throw std::runtime_error("CircuitNode::registerOutputVars not implemented");
}

}  // namespace atlantis::invariantgraph
