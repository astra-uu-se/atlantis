#include "atlantis/invariantgraph/invariantNodes/tableNode.hpp"

#include <algorithm>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/invariants/globalCardinalityOpen.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/invariants/table.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

TableNode::TableNode(InvariantGraph& graph,
                                             std::vector<VarNodeId>&& outputs,
                                             VarNodeId input,
                                             std::vector<std::vector<Int>>&& table,
                                             size_t inputColumnIndex)
    : InvariantNode(graph, std::move(outputs), {input}),
      _table(std::move(table)),
      _inputColumnIndex(inputColumnIndex) {
  assert(!_table.empty());
  assert(_table.front().size() == outputVarNodeIds().size());
}

void TableNode::init(InvariantNodeId id) {
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

size_t TableNode::numCols() const { return _table.front().size(); }

VarNodeId TableNode::colVar(size_t index) const {
  if (index == _inputColumnIndex) {
    return staticInputVarNodeIds().front();
  }
  return outputVarNodeIds().at(index - (index > _inputColumnIndex ? 1 : 0));
}

void TableNode::updateState() {
  // If the same variable occurs multiple times:
  std::vector<size_t> invalidRows;
  invalidRows.reserve(_table.size());
  for (size_t r = 0; r < _table.size(); ++r) {
    bool rowIsValid = true;
    for (size_t c1 = 0; rowIsValid && c1 < numCols(); ++c1) {
      if (!invariantGraphConst().varNodeConst(colVar(c1)).constDomain()->contains(_table[r][c1])) {
        invalidRows.emplace_back(r);
        rowIsValid = false;
        break;
      }
      for (size_t c2 = c1 + 1; rowIsValid && c2 < numCols(); ++c2) {
        if (colVar(c1) != colVar(c2)) {
          continue;
        }
        if (_table[r][c1] != _table[r][c2]) {
          invalidRows.emplace_back(r);
          rowIsValid = false;
          break;
        }
      }
    }
  }
  for (Int index = static_cast<Int>(invalidRows.size()) - 1; index >= 0; --index) {
    const size_t invalidRow = invalidRows[index];
    std::swap(_table.back(), _table[invalidRow]);
    _table.pop_back();
  }
}

bool TableNode::canBeReplaced() const {
  return false;
}

bool TableNode::replace() {
  return false;
}

void TableNode::registerOutputVars(propagation::SolverBase& solver,
                                               SolverMapping& mapping) const {
  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    assert(std::ranges::none_of(
        outputVarNodeIds().begin(),
        outputVarNodeIds().begin() + static_cast<Int>(i),
        [&](const VarNodeId vId) { return vId == outputVarNodeIds().at(i); }));

    assert(mapping.solverId(outputVarNodeIds().at(i)) ==
           propagation::NULL_ID);
    makeSolverVar(outputVarNodeIds().at(i), solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void TableNode::registerNode(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  const propagation::VarViewId inputVarId = mapping.solverId(staticInputVarNodeIds().front());

  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(outputVarNodeIds().size());
  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    assert(mapping.intermediateId(id(), i).isVar());

    outputVarIds.emplace_back(mapping.intermediateId(id(), i) ==
                                      mapping.solverId(outputVarNodeIds().at(i)));
  }

  solver.makeInvariant<propagation::Table>(
      solver, std::move(outputVarIds), inputVarId,
      std::vector<std::vector<Int>>(_table), _inputColumnIndex);
}

std::string TableNode::dotLangIdentifier() const {
  return {"table"};
}

}  // namespace atlantis::invariantgraph
