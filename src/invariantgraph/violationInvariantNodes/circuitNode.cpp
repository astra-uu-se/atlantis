#include "atlantis/invariantgraph/violationInvariantNodes/circuitNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/invariantgraph/views/intModViewNode.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

namespace atlantis::invariantgraph {

CircuitNode::CircuitNode(IInvariantGraph& graph, std::vector<VarNodeId>&& vars,
                         Int offset)
    : ViolationInvariantNode(graph, std::move(vars), true), _offset(offset) {}

void CircuitNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void CircuitNode::updateState() {
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph()
        .varNode(staticInputVarNodeIds().front())
        .fixToValue(Int{1});
  } else if (staticInputVarNodeIds().size() == 2) {
    invariantGraph()
        .varNode(staticInputVarNodeIds().front())
        .fixToValue(Int{2});
    invariantGraph().varNode(staticInputVarNodeIds().back()).fixToValue(Int{1});
  }
  if (staticInputVarNodeIds().size() <= 2) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool CircuitNode::canBeMadeImplicit() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         shouldHold() && staticInputVarNodeIds().size() > 2 &&
         std::all_of(
             staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
             [&](const VarNodeId nId) {
               return invariantGraphConst().varNodeConst(nId).isFixed() ||
                      invariantGraphConst()
                          .varNodeConst(nId)
                          .definingNodes()
                          .empty();
             });
}

bool CircuitNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }
  invariantGraph().addImplicitConstraintNode(
      std::make_shared<CircuitImplicitNode>(
          invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
          _offset));
  return true;
}

bool CircuitNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && !canBeMadeImplicit();
}

bool CircuitNode::replace() {
  if (!canBeReplaced()) {
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
  invariantGraph().addInvariantNode(std::make_shared<AllDifferentNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()}, true));

  std::vector<VarNodeId> orderVars;
  orderVars.reserve(staticInputVarNodeIds().size());
  std::vector<VarNodeId> offsetVars;
  offsetVars.reserve(staticInputVarNodeIds().size());
  std::vector<VarNodeId> modoluVars;
  modoluVars.reserve(staticInputVarNodeIds().size());
  // order[l] == 1:
  orderVars.emplace_back(invariantGraph().retrieveIntVarNode(1));
  // order[l] == 1 -> offset[0] = 2
  offsetVars.emplace_back(invariantGraph().retrieveIntVarNode(2));
  // order[l] == 1 -> modulo[0] = 2 mod n
  modoluVars.emplace_back(invariantGraph().retrieveIntVarNode(
      2 % static_cast<Int>(staticInputVarNodeIds().size())));

  for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
    orderVars.emplace_back(invariantGraph().retrieveIntVarNode(
        SearchDomain{1, static_cast<Int>(staticInputVarNodeIds().size())},
        VarNode::DomainType::NONE));
    offsetVars.emplace_back(invariantGraph().retrieveIntVarNode(
        SearchDomain{1, static_cast<Int>(staticInputVarNodeIds().size()) + 1},
        VarNode::DomainType::NONE));
    modoluVars.emplace_back(invariantGraph().retrieveIntVarNode(
        SearchDomain{0, static_cast<Int>(staticInputVarNodeIds().size()) - 1},
        VarNode::DomainType::NONE));
    // offset[i] = order[i] + 1
    invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(
        invariantGraph(), offsetVars.at(i), orderVars.at(i), 1, 1));
    // modulo[i] = offset[i] mod n
    invariantGraph().addInvariantNode(std::make_shared<IntModViewNode>(
        invariantGraph(), modoluVars.at(i), offsetVars.at(i),
        staticInputVarNodeIds().size()));
  }
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    std::vector<VarNodeId> orderSansI;
    orderSansI.reserve(staticInputVarNodeIds().size());
    for (size_t j = 0; j < staticInputVarNodeIds().size(); ++j) {
      if (i == j) {
        // remove self cycle:
        orderSansI.emplace_back(invariantGraph().retrieveIntVarNode(1));
      } else {
        orderSansI.emplace_back(orderVars.at(j));
      }
    }
    // sansI[x[i]] = modulo[i]
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), staticInputVarNodeIds().at(i), std::move(orderSansI),
        modoluVars.at(i), _offset));
  }

  invariantGraph().addInvariantNode(std::make_shared<AllDifferentNode>(
      invariantGraph(), std::move(orderVars), true));

  return true;
}

void CircuitNode::registerOutputVars() {
  throw std::runtime_error("CircuitNode::registerOutputVars not implemented");
}

void CircuitNode::registerNode() {
  throw std::runtime_error("CircuitNode::registerOutputVars not implemented");
}

}  // namespace atlantis::invariantgraph
