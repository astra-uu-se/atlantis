#include "atlantis/invariantgraph/invariantNodes/blackboxNode.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/misc/blackboxFunction.hpp"
#include "atlantis/propagation/invariants/blackbox.hpp"

namespace atlantis::invariantgraph {

BlackBoxNode::BlackBoxNode(InvariantGraph& graph,
                           std::shared_ptr<blackbox::BlackBoxFn> blackBoxFn,
                           std::vector<VarNodeId>&& intIn,
                           std::vector<VarNodeId>&& intOut)
    : InvariantNode(graph, std::move(intOut), std::move(intIn)),
      _blackBoxFn(std::move(blackBoxFn)) {}

void BlackBoxNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BlackBoxNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  for (const auto& outputVarNodeId : outputVarNodeIds()) {
    makeSolverVar(outputVarNodeId, solver, mapping);
  }
  assert(std::ranges::all_of(outputVarNodeIds().begin(),
                             outputVarNodeIds().end(),
                             [&](const VarNodeId vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void BlackBoxNode::registerNode(propagation::SolverBase& solver,
                                SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> inputVarIds;
  inputVarIds.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(staticInputVarNodeIds(),
                         std::back_inserter(inputVarIds),
                         [&](const VarNodeId varNodeId) {
                           assert(mapping.solverId(varNodeId) !=
                                  propagation::NULL_ID);
                           return mapping.solverId(varNodeId);
                         });

  std::vector<propagation::VarId> outputVarIds;
  outputVarIds.reserve(outputVarNodeIds().size());
  std::ranges::transform(outputVarNodeIds(), std::back_inserter(outputVarIds),
                         [&](const VarNodeId varNodeId) {
                           const propagation::VarViewId id =
                               mapping.solverId(varNodeId);
                           assert(id.isVar());
                           return propagation::VarId{static_cast<size_t>(id)};
                         });

  solver.makeInvariant<propagation::Blackbox>(
      solver, _blackBoxFn, std::move(outputVarIds), std::move(inputVarIds));
}

std::string BlackBoxNode::dotLangIdentifier() const { return "blackbox"; }

}  // namespace atlantis::invariantgraph
