#include "parseHelper.hpp"

namespace atlantis::invariantgraph {

bool hasCorrectSignature(
    const std::vector<std::pair<std::string, size_t>> &nameNumArgPairs,
    const fznparser::Constraint &constraint) {
  std::any_of(
      nameNumArgPairs.begin(), nameNumArgPairs.end(),
      [&](const std::pair<std::string, size_t>& p) {
        return p.first == constraint.identifier() && p.second == constraint.arguments().size();
      });
}

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
    InvariantGraph &invariantGraph, std::vector<VarNodeId> inputs) {
  // pruned[i] = <index, value> where index is the index of the static
  // variable with singleton domain {value}.
  std::vector<std::pair<size_t, Int>> fixed;
  fixed.reserve(inputs.size());

  for (size_t i = 0; i < inputs.size(); ++i) {
    for (const auto &[index, value] : fixed) {
      // remove all fixed values from the current variable:
      assert(index < i);
      invariantGraph.varNode(inputs[i]).removeValue(value);
    }
    if (!invariantGraph.varNode(inputs[i]).isFixed()) {
      continue;
    }
    // the variable has a singleton domain
    // Remove all occurrences of the value from previous static variables. Any
    // variable that gets a singleton domain is added to the fixed list.
    fixed.emplace_back(i, invariantGraph.varNode(inputs[i]).val());
    for (size_t p = fixed.size() - 1; p < fixed.size(); ++p) {
      const auto &[index, value] = fixed.at(p);
      for (size_t j = 0; j < index; j++) {
        const bool wasConstant = invariantGraph.varNode(inputs[j]).isFixed();
        invariantGraph.varNode(inputs[j]).removeValue(value);
        if (!wasConstant && invariantGraph.varNode(inputs[j]).isFixed()) {
          fixed.emplace_back(j, invariantGraph.varNode(inputs[j]).val());
        }
      }
    }
  }
  return fixed;
}

std::vector<VarNodeId> pruneAllDifferentFree(InvariantGraph &invariantGraph,
                                             std::vector<VarNodeId> inputs) {
  const auto fixed = allDifferent(invariantGraph, inputs);
  std::vector<bool> isFree(inputs.size(), true);
  for (const auto &[index, _] : fixed) {
    isFree[index] = false;
  }
  std::vector<VarNodeId> freeVars;
  freeVars.reserve(inputs.size() - fixed.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (isFree[i]) {
      freeVars.push_back(inputs[i]);
    }
  }
  return freeVars;
}

std::vector<VarNodeId> pruneAllDifferentFixed(InvariantGraph &invariantGraph,
                                              std::vector<VarNodeId> inputs) {
  const auto fixed = allDifferent(invariantGraph, inputs);
  std::vector<bool> isFree(inputs.size(), true);
  for (const auto &[index, _] : fixed) {
    isFree[index] = false;
  }
  std::vector<VarNodeId> freeVars;
  freeVars.reserve(inputs.size() - fixed.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (!isFree[i]) {
      freeVars.push_back(inputs[i]);
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