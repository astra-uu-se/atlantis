#include "parseHelper.hpp"

#include <algorithm>
#include <fznparser/model.hpp>
#include <ranges>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"

namespace atlantis::invariantgraph {

std::vector<VarNodeId>&& append(std::vector<VarNodeId>&& vars, VarNodeId fst,
                                VarNodeId snd) {
  if (fst != NULL_NODE_ID) {
    vars.emplace_back(fst);
  }
  if (snd != NULL_NODE_ID) {
    vars.emplace_back(snd);
  }
  return std::move(vars);
}

std::vector<VarNodeId>&& append(std::vector<VarNodeId>&& vars, VarNodeId var) {
  if (var != NULL_NODE_ID) {
    vars.emplace_back(var);
  }
  return std::move(vars);
}

std::vector<VarNodeId> concat(const std::vector<VarNodeId>& fst,
                              const std::vector<VarNodeId>& snd) {
  std::vector<VarNodeId> res;
  res.reserve(fst.size() + snd.size());
  res.insert(res.end(), fst.begin(), fst.end());
  res.insert(res.end(), snd.begin(), snd.end());
  return res;
}

static std::vector<std::pair<size_t, Int>> allDifferent(
    InvariantGraph& invariantGraph,
    const std::vector<VarNodeId>& inputVarNodeIds) {
  // pruned[i] = <index, value> where index is the index of the static
  // variable with singleton domain {value}.
  std::vector<std::pair<size_t, Int>> fixed;
  fixed.reserve(inputVarNodeIds.size());

  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    for (const auto& [index, value] : fixed) {
      // remove all fixed values from the current variable:
      assert(index < i);
      invariantGraph.varNode(inputVarNodeIds[i]).removeValue(value);
    }
    if (!invariantGraph.varNode(inputVarNodeIds[i]).isFixed()) {
      continue;
    }
    // the variable has a singleton domain
    // Remove all occurrences of the value from previous static variables. Any
    // variable that gets a singleton domain is added to the fixed list.
    fixed.emplace_back(i, invariantGraph.varNode(inputVarNodeIds[i]).val());
    for (size_t p = fixed.size() - 1; p < fixed.size(); ++p) {
      const auto& [index, value] = fixed.at(p);
      for (size_t j = 0; j < index; j++) {
        const bool wasConstant =
            invariantGraph.varNode(inputVarNodeIds[j]).isFixed();
        invariantGraph.varNode(inputVarNodeIds[j]).removeValue(value);
        if (!wasConstant &&
            invariantGraph.varNode(inputVarNodeIds[j]).isFixed()) {
          fixed.emplace_back(j,
                             invariantGraph.varNode(inputVarNodeIds[j]).val());
        }
      }
    }
  }
  return fixed;
}

std::vector<VarNodeId> pruneAllDifferentFree(
    InvariantGraph& invariantGraph,
    const std::vector<VarNodeId>& inputVarNodeIds) {
  const auto fixed = allDifferent(invariantGraph, inputVarNodeIds);
  std::vector<bool> isFree(inputVarNodeIds.size(), true);
  for (const auto& index : std::views::keys(fixed)) {
    isFree[index] = false;
  }
  std::vector<VarNodeId> freeVars;
  freeVars.reserve(inputVarNodeIds.size() - fixed.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    if (isFree[i]) {
      freeVars.push_back(inputVarNodeIds[i]);
    }
  }
  return freeVars;
}

std::vector<VarNodeId> pruneAllDifferentFixed(
    InvariantGraph& invariantGraph,
    const std::vector<VarNodeId>& inputVarNodeIds) {
  const auto fixed = allDifferent(invariantGraph, inputVarNodeIds);
  std::vector<VarNodeId> fixedVars;
  fixedVars.reserve(fixed.size());
  for (const size_t index : std::views::keys(fixed)) {
    fixedVars.emplace_back(inputVarNodeIds[index]);
    assert(invariantGraph.varNodeConst(fixedVars.back()).isFixed());
  }
  return fixedVars;
}

std::vector<Int> toIntVector(const std::vector<bool>& argument) {
  std::vector<Int> ints;
  ints.reserve(argument.size());
  std::ranges::transform(argument, std::back_inserter(ints),
                         [](const bool b) { return 1 - static_cast<Int>(b); });

  return ints;
}

bool removeFirstOccurrence(std::vector<size_t>& vector, size_t val) {
  for (size_t i = 0; i < vector.size(); i++) {
    if (vector[i] == val) {
      vector[i] = vector.back();
      vector.pop_back();
      return true;
    }
  }
  return false;
}

}  // namespace atlantis::invariantgraph