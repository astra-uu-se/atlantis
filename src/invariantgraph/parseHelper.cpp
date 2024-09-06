#include "parseHelper.hpp"

#include <algorithm>
#include <fznparser/model.hpp>
#include <optional>
#include <variant>

namespace atlantis::invariantgraph {

std::vector<invariantgraph::VarNodeId> &&append(
    std::vector<invariantgraph::VarNodeId> &&vars,
    invariantgraph::VarNodeId fst, invariantgraph::VarNodeId snd) {
  if (fst != NULL_NODE_ID) {
    vars.emplace_back(fst);
  }
  if (snd != NULL_NODE_ID) {
    vars.emplace_back(snd);
  }
  return std::move(vars);
}

std::vector<invariantgraph::VarNodeId> &&append(
    std::vector<invariantgraph::VarNodeId> &&vars,
    invariantgraph::VarNodeId var) {
  if (var != NULL_NODE_ID) {
    vars.emplace_back(var);
  }
  return std::move(vars);
}

std::vector<invariantgraph::VarNodeId> concat(
    const std::vector<invariantgraph::VarNodeId> &fst,
    const std::vector<invariantgraph::VarNodeId> &snd) {
  std::vector<invariantgraph::VarNodeId> res;
  res.reserve(fst.size() + snd.size());
  res.insert(res.end(), fst.begin(), fst.end());
  res.insert(res.end(), snd.begin(), snd.end());
  return res;
}

static std::vector<std::pair<size_t, Int>> allDifferent(
    IInvariantGraph &invariantGraph, std::vector<VarNodeId> inputVarNodeIds) {
  // pruned[i] = <index, value> where index is the index of the static
  // variable with singleton domain {value}.
  std::vector<std::pair<size_t, Int>> fixed;
  fixed.reserve(inputVarNodeIds.size());

  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    for (const auto &[index, value] : fixed) {
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
      const auto &[index, value] = fixed.at(p);
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
    IInvariantGraph &invariantGraph, std::vector<VarNodeId> inputVarNodeIds) {
  const auto fixed = allDifferent(invariantGraph, inputVarNodeIds);
  std::vector<bool> isFree(inputVarNodeIds.size(), true);
  for (const auto &[index, _] : fixed) {
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
    IInvariantGraph &invariantGraph, std::vector<VarNodeId> inputVarNodeIds) {
  const auto fixed = allDifferent(invariantGraph, inputVarNodeIds);
  std::vector<bool> isFree(inputVarNodeIds.size(), true);
  for (const auto &[index, _] : fixed) {
    isFree[index] = false;
  }
  std::vector<VarNodeId> freeVars;
  freeVars.reserve(inputVarNodeIds.size() - fixed.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    if (!isFree[i]) {
      freeVars.push_back(inputVarNodeIds[i]);
    }
  }
  return freeVars;
}

std::vector<Int> toIntVector(const std::vector<bool> &argument) {
  std::vector<Int> ints;
  ints.reserve(argument.size());
  std::transform(argument.begin(), argument.end(), std::back_inserter(ints),
                 [](const bool b) { return 1 - static_cast<Int>(b); });

  return ints;
}

}  // namespace atlantis::invariantgraph