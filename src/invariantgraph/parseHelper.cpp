#include "parseHelper.hpp"

#include <algorithm>
#include <fznparser/model.hpp>
#include <ranges>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/greaterEqualConst.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "atlantis/propagation/violationInvariants/lessThan.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

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
std::vector<ConstraintVarId> toConstraintVarIds(
    const InvariantGraph& invariantGraph,
    const std::vector<VarNodeId>& varNodeIds) {
  std::vector<ConstraintVarId> constraintVarIds(varNodeIds.size(),
                                                ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < varNodeIds.size(); ++i) {
    constraintVarIds[i] =
        invariantGraph.varNodeConst(varNodeIds[i]).constraintVarId();
  }
  return constraintVarIds;
}

void postAllEqualOnReplacedVars(
    InvariantGraph& invariantGraph,
    std::vector<std::pair<VarNodeId, VarNodeId>>&& replacedVarNodeIds) {
  while (!replacedVarNodeIds.empty()) {
    const auto [oldVarNodeId, newVarNodeId] = replacedVarNodeIds.front();
    assert(oldVarNodeId != newVarNodeId);
    std::vector<VarNodeId> duplicates;
    duplicates.reserve(2 * replacedVarNodeIds.size());
    duplicates.emplace_back(oldVarNodeId);
    duplicates.emplace_back(newVarNodeId);
    for (size_t i = replacedVarNodeIds.size() - 1; i > 0; i--) {
      if (replacedVarNodeIds[i].first != oldVarNodeId) {
        continue;
      }
      duplicates.emplace_back(replacedVarNodeIds[i].second);
      std::swap(replacedVarNodeIds[i], replacedVarNodeIds.back());
      replacedVarNodeIds.pop_back();
    }
    std::swap(replacedVarNodeIds.front(), replacedVarNodeIds.back());
    replacedVarNodeIds.pop_back();
    if (!invariantGraph.varNodeConst(oldVarNodeId).isFixed()) {
      if (invariantGraph.varNodeConst(oldVarNodeId).isIntVar()) {
        invariantGraph.addInvariantNode(std::make_shared<IntAllEqualNode>(
            invariantGraph, std::move(duplicates), true));
      } else {
        invariantGraph.addInvariantNode(std::make_shared<BoolAllEqualNode>(
            invariantGraph, std::move(duplicates), true));
      }
    }
  }
}

std::pair<std::vector<VarNodeId>, SortedUniqueVector> gccUpdateState(
    const InvariantGraph& invariantGraph, const std::vector<VarNodeId>& inputs,
    const std::vector<Int>& cover) {
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(inputs.size());

  std::vector<bool> coverIntersectsDomains(cover.size(), false);

  for (const VarNodeId vId : inputs) {
    bool domainIntersectsCover = false;
    for (size_t coverIndex = 0; coverIndex < cover.size(); ++coverIndex) {
      if (invariantGraph.varNodeConst(vId).isFixed()) {
        varsToRemove.emplace_back(vId);
        break;
      }
      if (invariantGraph.varNodeConst(vId).inDomain(cover[coverIndex])) {
        coverIntersectsDomains[coverIndex] = true;
        domainIntersectsCover = true;
      }
    }
    if (!domainIntersectsCover) {
      varsToRemove.emplace_back(vId);
    }
  }

  std::vector<Int> coverIndicesToRemove;
  coverIndicesToRemove.reserve(cover.size());

  for (size_t i = 0; i < cover.size(); ++i) {
    if (!coverIntersectsDomains[i]) {
      coverIndicesToRemove.emplace_back(i);
    }
  }

  return std::pair<std::vector<VarNodeId>, SortedUniqueVector>{
      varsToRemove, SortedUniqueVector(std::move(coverIndicesToRemove))};
}

RelationType getRelType(const RelationType relType, const bool shouldHold,
                        const bool swapSides) {
  const RelationType rt =
      shouldHold ? relType : relationTypeComplement(relType);
  return swapSides ? relationTypeConverse(rt) : rt;
}

propagation::VarViewId makeSolverConstIntRelation(propagation::SolverBase& solver,
                                           const propagation::VarViewId lhs,
                                           const RelationType relType,
                                           const Int rhs, const bool shouldHold,
                                           const bool swapSides) {
  switch (getRelType(relType, shouldHold, swapSides)) {
    case RelationType::REL_TYPE_EQ:
      return solver.makeIntView<propagation::EqualConst>(solver, lhs, rhs);
    case RelationType::REL_TYPE_NE:
      return solver.makeIntView<propagation::NotEqualConst>(solver, lhs, rhs);
    case RelationType::REL_TYPE_GE:
      return solver.makeIntView<propagation::GreaterEqualConst>(solver, lhs,
                                                                rhs);
    case RelationType::REL_TYPE_GT:
      return solver.makeIntView<propagation::GreaterEqualConst>(solver, lhs,
                                                                rhs + 1);
    case RelationType::REL_TYPE_LT:
      return solver.makeIntView<propagation::LessEqualConst>(solver, lhs,
                                                             rhs - 1);
    case RelationType::REL_TYPE_LE:
      return solver.makeIntView<propagation::LessEqualConst>(solver, lhs, rhs);
  }
  return propagation::NULL_ID;
}

propagation::VarViewId makeSolverConstBoolRelation(
    propagation::SolverBase& solver, const propagation::VarViewId lhs,
    const RelationType relType, const bool rhs, const bool shouldHold,
    const bool swapSides) {
  constexpr unsigned char isFalse = 0;
  constexpr unsigned char isTrue = 1;
  constexpr unsigned char none = 2;
  unsigned char viewType = none;
  const RelationType rt = getRelType(relType, shouldHold, swapSides);
  if (rt == RelationType::REL_TYPE_EQ) {
    viewType = rhs ? isTrue : isFalse;
  } else if (rt == RelationType::REL_TYPE_NE) {
    viewType = rhs ? isFalse : isTrue;
  } else if (rt == RelationType::REL_TYPE_GT) {
    assert(!rhs);
    viewType = isTrue;
  } else if (rt == RelationType::REL_TYPE_GE && rhs) {
    viewType = isTrue;
  } else if (rt == RelationType::REL_TYPE_LE && !rhs) {
    viewType = isFalse;
  } else if (rt == RelationType::REL_TYPE_LT) {
    assert(rhs);
    viewType = isFalse;
  }
  if (viewType == isTrue) {
    return solver.makeIntView<propagation::EqualConst>(solver, lhs, 0);
  }
  if (viewType == isFalse) {
    return solver.makeIntView<propagation::NotEqualConst>(solver, lhs, 0);
  }
  return propagation::NULL_ID;
}

void makeSolverIntRelation(propagation::SolverBase& solver,
                        const propagation::VarViewId lhs,
                        const RelationType relType,
                        const propagation::VarViewId rhs,
                        const propagation::VarViewId violation,
                        const bool shouldHold) {
  switch (shouldHold ? relType : relationTypeComplement(relType)) {
    case RelationType::REL_TYPE_EQ:
      solver.makeViolationInvariant<propagation::Equal>(solver, violation, lhs,
                                                        rhs);
      break;
    case RelationType::REL_TYPE_NE:
      solver.makeViolationInvariant<propagation::NotEqual>(solver, violation,
                                                           lhs, rhs);
      break;
    case RelationType::REL_TYPE_GE:
      solver.makeViolationInvariant<propagation::LessEqual>(solver, violation,
                                                            rhs, lhs);
      break;
    case RelationType::REL_TYPE_GT:
      solver.makeViolationInvariant<propagation::LessThan>(solver, violation,
                                                           rhs, lhs);
      break;
    case RelationType::REL_TYPE_LT:
      solver.makeViolationInvariant<propagation::LessThan>(solver, violation,
                                                           lhs, rhs);
      break;
    case RelationType::REL_TYPE_LE:
      solver.makeViolationInvariant<propagation::LessEqual>(solver, violation,
                                                            lhs, rhs);
      break;
  }
}

void makeSolverBoolRelation(propagation::SolverBase& solver,
                            const propagation::VarViewId lhs,
                            const RelationType relType,
                            const propagation::VarViewId rhs,
                            const propagation::VarViewId violation,
                            const bool shouldHold) {
  switch (shouldHold ? relType : relationTypeComplement(relType)) {
    case RelationType::REL_TYPE_EQ:
      solver.makeViolationInvariant<propagation::BoolEqual>(solver, violation,
                                                            lhs, rhs);
      break;
    case RelationType::REL_TYPE_NE:
      solver.makeInvariant<propagation::BoolXor>(solver, violation, lhs, rhs);
      break;
    case RelationType::REL_TYPE_GE:
      solver.makeViolationInvariant<propagation::BoolLessEqual>(
          solver, violation, rhs, lhs);
      break;
    case RelationType::REL_TYPE_GT:
      solver.makeViolationInvariant<propagation::BoolLessThan>(
          solver, violation, rhs, lhs);
      break;
    case RelationType::REL_TYPE_LT:
      solver.makeViolationInvariant<propagation::BoolLessThan>(
          solver, violation, lhs, rhs);
      break;
    case RelationType::REL_TYPE_LE:
      solver.makeViolationInvariant<propagation::BoolLessEqual>(
          solver, violation, lhs, rhs);
      break;
  }
}

bool violToBool(const Int violation) { return violation == 0; }

std::vector<bool> violToBool(const std::vector<Int>& violations) {
  std::vector<bool> bools(violations.size());
  for (size_t i = 0; i < violations.size(); ++i) {
    bools[i] = violations[i] == 0;
  }
  return bools;
}

std::vector<std::vector<bool>> violToBool(
    const std::vector<std::vector<Int>>& violations) {
  std::vector<std::vector<bool>> bools(violations.size());
  for (size_t i = 0; i < violations.size(); ++i) {
    bools[i] = violToBool(violations[i]);
  }
  return bools;
}

Int boolToViol(const Int b) { return b ? 0 : 1; }

std::vector<Int> boolToViol(const std::vector<bool>& bools) {
  std::vector<Int> violations(bools.size());
  for (size_t i = 0; i < bools.size(); ++i) {
    violations[i] = bools[i] ? 0 : 1;
  }
  return violations;
}

std::vector<std::vector<Int>> boolToViol(
    const std::vector<std::vector<bool>>& bools) {
  std::vector<std::vector<Int>> violations(bools.size());
  for (size_t i = 0; i < bools.size(); ++i) {
    violations[i] = boolToViol(bools[i]);
  }
  return violations;
}

std::pair<std::vector<VarNodeId>, SortedUniqueVector> gccUpdateState(
    const InvariantGraph& invariantGraph, const std::vector<VarNodeId>& inputs,
    const std::vector<Int>& cover, std::vector<Int>& lowerBounds,
    std::vector<Int>& upperBounds) {
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(inputs.size());

  std::vector<bool> coverIntersectsDomains(cover.size(), false);

  for (const VarNodeId vId : inputs) {
    bool domainIntersectsCover = false;
    for (size_t coverIndex = 0; coverIndex < cover.size(); ++coverIndex) {
      if (invariantGraph.varNodeConst(vId).isFixed()) {
        if (invariantGraph.varNodeConst(vId).lowerBound() ==
            cover[coverIndex]) {
          --lowerBounds[coverIndex];
          --upperBounds[coverIndex];
        }
        varsToRemove.emplace_back(vId);
        break;
      }
      if (invariantGraph.varNodeConst(vId).inDomain(cover[coverIndex])) {
        coverIntersectsDomains[coverIndex] = true;
        domainIntersectsCover = true;
      }
    }
    if (!domainIntersectsCover) {
      varsToRemove.emplace_back(vId);
    }
  }

  std::vector<Int> coverIndicesToRemove;
  coverIndicesToRemove.reserve(cover.size());

  for (Int i = static_cast<Int>(cover.size()) - 1; i >= 0; --i) {
    if (!coverIntersectsDomains[i]) {
      coverIndicesToRemove.emplace_back(i);
    }
  }

  return std::pair<std::vector<VarNodeId>, SortedUniqueVector>{
      varsToRemove, SortedUniqueVector(std::move(coverIndicesToRemove))};
}

Int maxOverlaps(const std::vector<std::pair<Int, Int> > &intervals) {
  // for each element {coordinate, type} in data, coordinate is a start or end coordinate, and type = false for 'start', true for 'end'
  std::vector<std::pair<Int, bool>> data;
  data.reserve(intervals.size() * 2);

  // Store start and end coordinates:
  for (const auto &[start, end] : intervals) {
    data.emplace_back(start, false);
    data.emplace_back(end, true);
  }

  // Sort increasingly by coordinate; start comes before end if coordinates are equal:
  std::ranges::sort(data, [](const std::pair<Int, bool> &a, const std::pair<Int, bool> &b) {
      return (a.first != b.first) ? a.first < b.first : (a.second ? 1 : 0) < (b.second ? 1 : 0);
  });

  // Count overlaps
  size_t ans = 0;
  size_t count = 0;
  for (const auto &val: std::views::values(data)) {
    // start
    count += val ? 0 : 1;
    ans = std::max(ans, count);
  }

  return ans;
}

}  // namespace atlantis::invariantgraph