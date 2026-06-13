#include "atlantis/invariantgraph/gecodeSolver.hpp"

#include <gecode/minimodel.hh>
#include <libs/log/src/murmur3.hpp>
#include <numeric>
#include <ranges>
#include <vector>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/gecodeSolver.hpp"

namespace atlantis::invariantgraph {

Gecode::IntRelType toGecodeIntRelType(const RelationType relType,
                                      const bool shouldHold = true) {
  switch (shouldHold ? relType : relationTypeComplement(relType)) {
    case RelationType::REL_TYPE_NE:
      return Gecode::IntRelType::IRT_NQ;
    case RelationType::REL_TYPE_LE:
      return Gecode::IntRelType::IRT_LQ;
    case RelationType::REL_TYPE_LT:
      return Gecode::IntRelType::IRT_LE;
    case RelationType::REL_TYPE_GE:
      return Gecode::IntRelType::IRT_GQ;
    case RelationType::REL_TYPE_GT:
      return Gecode::IntRelType::IRT_GR;
    case RelationType::REL_TYPE_EQ:
    default:
      return Gecode::IntRelType::IRT_EQ;
  }
}

GecodeSolver::GecodeSpace::GecodeSpace(GecodeSpace& space) : Space(space) {
  _iv.resize(space._iv.size());
  for (size_t i = 0; i < space._iv.size(); ++i) {
    _iv[i].update(*this, space._iv[i]);
  }
  _bv.resize(space._bv.size());
  for (size_t i = 0; i < space._bv.size(); ++i) {
    _bv[i].update(*this, space._bv[i]);
  }
}

Gecode::Space* GecodeSolver::GecodeSpace::copy() {
  return new GecodeSpace(*this);
}

Gecode::BoolVarArgs GecodeSolver::boolVarArgs(
    const std::vector<ConstraintVarId>& varIds) {
  Gecode::BoolVarArgs args(static_cast<int>(varIds.size()));
  for (int i = 0; i < static_cast<int>(varIds.size()); ++i) {
    assert(varIds.at(i).isBoolVar());
    args[i] = boolVar(size_t{varIds[i]});
  }
  return args;
}

Gecode::BoolVarArgs GecodeSolver::boolVarArgs(
    const std::vector<std::vector<ConstraintVarId>>& varIds) {
  const int numIntVars = std::accumulate(
      varIds.begin(), varIds.end(), int{0},
      [&](const int size, const std::vector<ConstraintVarId>& row) {
        return size + static_cast<int>(row.size());
      });
  Gecode::BoolVarArgs args(static_cast<int>(numIntVars));
  int i = 0;
  for (const auto& row : varIds) {
    for (auto varId : row) {
      assert(varId.isBoolVar());
      args[i++] = boolVar(size_t{varId});
    }
  }
  return args;
}

Gecode::BoolVar GecodeSolver::boolVar(const size_t varId) {
  assert(varId < _space._bv.size());
  return _space._bv[size_t{varId}];
}

Gecode::BoolVar GecodeSolver::boolVar(const ConstraintVarId varId) {
  assert(varId.isBoolVar());
  return boolVar(size_t{varId});
}

Gecode::IntVarArgs GecodeSolver::intVarArgs(
    const std::vector<ConstraintVarId>& varIds) {
  Gecode::IntVarArgs args(static_cast<int>(varIds.size()));
  for (int i = 0; i < static_cast<int>(varIds.size()); ++i) {
    assert(varIds.at(i).isIntVar());
    args[i] = intVar(size_t{varIds[i]});
  }
  return args;
}

Gecode::IntVarArgs GecodeSolver::intVarArgs(
    const std::vector<std::vector<ConstraintVarId>>& varIds) {
  const int numIntVars = std::accumulate(
      varIds.begin(), varIds.end(), int{0},
      [&](const int size, const std::vector<ConstraintVarId>& row) {
        return size + static_cast<int>(row.size());
      });
  Gecode::IntVarArgs args(static_cast<int>(numIntVars));
  int i = 0;
  for (const auto& row : varIds) {
    for (auto varId : row) {
      assert(varId.isIntVar());
      args[i++] = intVar(size_t{varId});
    }
  }
  return args;
}

Gecode::TupleSet GecodeSolver::tupleSet(
    const std::vector<std::vector<Int>>& table) {
  // Build TupleSet
  Gecode::TupleSet ts(
      static_cast<int>(table.empty() ? 0 : table.front().size()));
  for (const auto& row : table) {
    Gecode::IntArgs tuple(static_cast<int>(row.size()));
    for (size_t col = 0; col < row.size(); col++) {
      tuple[static_cast<int>(col)] = static_cast<int>(row[col]);
    }
    ts.add(tuple);
  }
  ts.finalize();

  return ts;
}

Gecode::TupleSet GecodeSolver::tupleSet(
    const std::vector<std::vector<bool>>& table) {
  // Build TupleSet
  Gecode::TupleSet ts(
      static_cast<int>(table.empty() ? 0 : table.front().size()));
  for (const auto& row : table) {
    Gecode::IntArgs tuple(static_cast<int>(row.size()));
    for (size_t col = 0; col < row.size(); col++) {
      tuple[static_cast<int>(col)] = row[col] ? 1 : 0;
    }
    ts.add(tuple);
  }
  ts.finalize();

  return ts;
}

Gecode::IntSet GecodeSolver::intSet(const SortedUniqueVector& values) {
  if (values->empty()) {
    return {};
  }
  if (values.isInterval()) {
    return Gecode::IntSet(static_cast<int>(values->front()),
                          static_cast<int>(values->back()));
  }
  Gecode::Region re;
  int* is = re.alloc<int>(values->size());
  for (Int i = static_cast<Int>(values->size()) - 1; i >= 0; --i) {
    is[i] = static_cast<int>(values[i]);
  }
  return Gecode::IntSet(is, static_cast<int>(values->size()));
}

Gecode::IntVar GecodeSolver::intVar(const size_t varId) {
  assert(varId < _space._iv.size());
  return _space._iv[varId];
}

Gecode::IntVar GecodeSolver::intVar(const ConstraintVarId varId) {
  assert(varId.isIntVar());
  return intVar(size_t{varId});
}

Gecode::IntSharedArray GecodeSolver::intSharedArray(
    const std::vector<Int>& valVector) {
  Gecode::IntSharedArray pars(static_cast<int>(valVector.size()));
  for (int i = 0; i < static_cast<int>(valVector.size()); ++i) {
    pars[i] = static_cast<int>(valVector[i]);
  }
  return pars;
}

Gecode::IntSharedArray GecodeSolver::intSharedArray(
    const std::vector<bool>& valVector) {
  Gecode::IntSharedArray pars(static_cast<int>(valVector.size()));
  for (int i = 0; i < static_cast<int>(valVector.size()); ++i) {
    pars[i] = valVector[i] ? 1 : 0;
  }
  return pars;
}

Gecode::IntArgs GecodeSolver::intArgs(const std::vector<Int>& valVector) {
  Gecode::IntArgs pars(static_cast<int>(valVector.size()));
  for (int i = 0; i < static_cast<int>(valVector.size()); ++i) {
    pars[i] = static_cast<int>(valVector[i]);
  }
  return pars;
}

Gecode::IntArgs GecodeSolver::intArgs(const std::vector<bool>& valVector) {
  Gecode::IntArgs pars(static_cast<int>(valVector.size()));
  for (int i = 0; i < static_cast<int>(valVector.size()); ++i) {
    pars[i] = valVector[i] ? 1 : 0;
  }
  return pars;
}

Gecode::IntSharedArray GecodeSolver::intSharedArray(
    const std::vector<std::vector<Int>>& valMatrix) {
  const int numIntVars =
      std::accumulate(valMatrix.begin(), valMatrix.end(), int{0},
                      [&](const int size, const std::vector<Int>& row) {
                        return size + static_cast<int>(row.size());
                      });
  Gecode::IntSharedArray args(numIntVars);
  int i = 0;
  for (const auto& row : valMatrix) {
    for (const auto val : row) {
      args[i++] = static_cast<int>(val);
    }
  }
  return args;
}

Gecode::IntSharedArray GecodeSolver::intSharedArray(
    const std::vector<std::vector<bool>>& valMatrix) {
  const int numIntVars =
      std::accumulate(valMatrix.begin(), valMatrix.end(), int{0},
                      [&](const int size, const std::vector<bool>& row) {
                        return size + static_cast<int>(row.size());
                      });
  Gecode::IntSharedArray args(numIntVars);
  int i = 0;
  for (const auto& row : valMatrix) {
    for (const auto val : row) {
      args[i++] = val ? 1 : 0;
    }
  }
  return args;
}

void GecodeSolver::array_bool_op(const std::vector<ConstraintVarId>& inputs,
                                 const ConstraintVarId reified,
                                 const Gecode::BoolOpType op) {
  Gecode::rel(_space, op, boolVarArgs(inputs), boolVar(reified),
              Gecode::IPL_BND);
}

void GecodeSolver::array_bool_op(const std::vector<ConstraintVarId>& inputs,
                                 const bool shouldHold,
                                 const Gecode::BoolOpType op) {
  Gecode::rel(_space, op, boolVarArgs(inputs), shouldHold ? 1 : 0,
              Gecode::IPL_BND);
}

void GecodeSolver::bool_op(const ConstraintVarId lhs, const ConstraintVarId rhs,
                           const ConstraintVarId reified,
                           const Gecode::BoolOpType op) {
  Gecode::rel(_space, boolVar(lhs), op, boolVar(rhs), boolVar(reified),
              Gecode::IPL_BND);
}

void GecodeSolver::bool_op(const ConstraintVarId lhs, const ConstraintVarId rhs,
                           const bool shouldHold, const Gecode::BoolOpType op) {
  Gecode::rel(_space, boolVar(lhs), op, boolVar(rhs), shouldHold,
              Gecode::IPL_BND);
}

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const RelationType relation, const Int rhs,
                                const ConstraintVarId reified) {
  const auto& reif = boolVar(reified);
  if (reif.assigned()) {
    return bool_lin_rel(coeffs, inputs, relation, rhs, reif.val() == 1);
  }
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
         toGecodeIntRelType(relation), static_cast<int>(rhs),
         Gecode::Reify(reif, Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const RelationType relation,
                                const ConstraintVarId rhs,
                                const Int rhsOffset) {
  if (rhsOffset == 0) {
    return linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
                  toGecodeIntRelType(relation), intVar(rhs));
  }
  const auto rhsVar = expr(_space, intVar(rhs) + static_cast<int>(rhsOffset));
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
         toGecodeIntRelType(relation), rhsVar);
}

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const RelationType relation, const Int rhs,
                                const bool shouldHold) {
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
         toGecodeIntRelType(relation, shouldHold), static_cast<int>(rhs));
}

void GecodeSolver::int_lin_rel(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const RelationType relation, const Int rhs,
                               const ConstraintVarId reified) {
  const auto& reif = boolVar(reified);
  if (reif.assigned()) {
    return int_lin_rel(coeffs, inputs, relation, rhs, reif.val() == 1);
  }
  linear(_space, intSharedArray(coeffs), intVarArgs(inputs),
         toGecodeIntRelType(relation), static_cast<int>(rhs),
         Gecode::Reify(reif, Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::int_lin_rel(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const RelationType relation,
                               const ConstraintVarId rhs, const Int rhsOffset) {
  if (rhsOffset == 0) {
    return linear(_space, intSharedArray(coeffs), intVarArgs(inputs),
                  toGecodeIntRelType(relation), intVar(rhs));
  }
  const auto offsetVar =
      expr(_space, intVar(rhs) + static_cast<int>(rhsOffset));
  linear(_space, intSharedArray(coeffs), intVarArgs(inputs),
         toGecodeIntRelType(relation), offsetVar);
}

void GecodeSolver::int_lin_rel(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const RelationType relation, const Int rhs,
                               const bool shouldHold) {
  linear(_space, intSharedArray(coeffs), intVarArgs(inputs),
         toGecodeIntRelType(relation, shouldHold), static_cast<int>(rhs));
}

Gecode::IntArgs GecodeSolver::gcc_get_cover(const Gecode::IntVarArgs& inputVars,
                                            const std::vector<Int>& cover,
                                            Gecode::IntVarArgs& countVars) {
  auto intArgCover = intArgs(cover);

  Gecode::Region region;
  const Gecode::IntSet coverSet(intArgCover);
  Gecode::IntSetRanges coverRanges(coverSet);
  auto* inputDomains = region.alloc<Gecode::IntVarRanges>(inputVars.size());
  for (int i = inputVars.size(); i--;) {
    inputDomains[i] = Gecode::IntVarRanges(inputVars[i]);
  }
  Gecode::Iter::Ranges::NaryUnion domainUnion(region, inputDomains,
                                              inputVars.size());
  Gecode::Iter::Ranges::Diff<Gecode::Iter::Ranges::NaryUnion,
                             Gecode::IntSetRanges>
      extraRanges(domainUnion, coverRanges);
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::Diff<
      Gecode::Iter::Ranges::NaryUnion, Gecode::IntSetRanges>>
      extra(extraRanges);
  for (; extra(); ++extra) {
    intArgCover << extra.val();
    countVars << Gecode::IntVar(_space, 0, inputVars.size());
  }
  return intArgCover;
}

bool GecodeSolver::allFixed(const std::vector<ConstraintVarId>& vars) {
  return std::ranges::all_of(vars, [this](const ConstraintVarId v) {
    return v.isBoolVar() ? boolVar(v).assigned() : intVar(v).assigned();
  });
}

bool GecodeSolver::isFixedTo(const ConstraintVarId b, const bool val) {
  return boolVar(b).assigned() && (boolVar(b).val() == 1) == val;
}

bool GecodeSolver::isFixedTo(const std::variant<bool, ConstraintVarId> b,
                             const bool val) {
  return std::holds_alternative<bool>(b)
             ? std::get<bool>(b) == val
             : isFixedTo(std::get<ConstraintVarId>(b), val);
}

bool GecodeSolver::gcc_cover_sanity(
    const std::vector<Int>& cover,
    const std::variant<bool, ConstraintVarId> reified) {
  if (!cover.empty()) {
    return false;
  }
  if (std::holds_alternative<bool>(reified)) {
    if (!isFixedTo(reified, true)) {
      throw InconsistencyException("UNSAT");
    }
    return true;
  }
  Gecode::rel(_space, boolVar(std::get<ConstraintVarId>(reified)),
              Gecode::IRT_EQ, 1);
  return true;
}

bool GecodeSolver::gcc_sanity(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts,
    const std::variant<bool, ConstraintVarId> reified) {
  if (!inputs.empty()) {
    return false;
  }
  if (cover.empty()) {
    if (isFixedTo(reified, false)) {
      throw InconsistencyException("UNSAT");
    }
    if (!isFixedTo(reified, true)) {
      Gecode::rel(_space, boolVar(std::get<ConstraintVarId>(reified)),
                  Gecode::IRT_EQ, 1);
    }
    return true;
  }

  if (isFixedTo(reified, true)) {
    for (const auto c : counts) {
      Gecode::rel(_space, intVar(c), Gecode::IRT_EQ, 0);
    }
    return true;
  }
  if (isFixedTo(reified, false)) {
    Gecode::count(_space, intVarArgs(inputs), 0, Gecode::IRT_LE,
                  static_cast<int>(counts.size()));
    return true;
  }
  Gecode::BoolVarArgs areZero(static_cast<int>(cover.size()));
  for (int i = 0; i < areZero.size(); i++) {
    areZero[i] = Gecode::BoolVar(_space, 0, 1);
    Gecode::rel(_space, intVar(counts[i]), Gecode::IRT_EQ, 0, areZero[i]);
  }
  if (std::holds_alternative<bool>(reified)) {
    Gecode::rel(_space, Gecode::BOT_AND, areZero, 0);
  } else {
    Gecode::rel(_space, Gecode::BOT_AND, areZero,
                boolVar(std::get<ConstraintVarId>(reified)));
  }
  return true;
}

bool GecodeSolver::gcc_closed_sanity(
    const std::vector<Int>& cover,
    const std::variant<bool, ConstraintVarId> reified) {
  if (!cover.empty()) {
    return false;
  }
  if (isFixedTo(reified, true)) {
    throw InconsistencyException("UNSAT");
  }
  if (!isFixedTo(reified, false)) {
    Gecode::rel(_space, boolVar(std::get<ConstraintVarId>(reified)),
                Gecode::IRT_EQ, 0);
  }
  return true;
}

bool GecodeSolver::gcc_sanity(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const std::variant<bool, ConstraintVarId> reified) {
  if (!inputs.empty() && !cover.empty()) {
    return false;
  }
  if (gcc_cover_sanity(cover, reified)) {
    return true;
  }

  size_t satisfiedCounts = 0;
  for (size_t i = 0; i < cover.size(); i++) {
    if (lowerBounds[i] <= 0 && 0 <= upperBounds[i]) {
      ++satisfiedCounts;
    }
  }
  if (isFixedTo(reified, true)) {
    if (satisfiedCounts < cover.size()) {
      throw InconsistencyException("UNSAT");
    }
  }
  if (isFixedTo(reified, false)) {
    if (satisfiedCounts == cover.size()) {
      throw InconsistencyException("UNSAT");
    }
  }
  Gecode::rel(_space, boolVar(std::get<ConstraintVarId>(reified)),
              Gecode::IRT_EQ, satisfiedCounts == cover.size() ? 1 : 0);
  return true;
}

std::pair<std::vector<Int>, std::vector<ConstraintVarId>>
GecodeSolver::gcc_combine_covers(
    const std::vector<Int>& cover, const std::vector<ConstraintVarId>& counts,
    const std::variant<bool, ConstraintVarId> reified) {
  std::vector<Int> newCover;
  newCover.reserve(cover.size());
  std::vector<ConstraintVarId> newCounts;
  newCounts.reserve(counts.size());

  std::vector<bool> removed(cover.size(), false);
  for (size_t i = 0; i < cover.size(); i++) {
    if (removed[i]) {
      continue;
    }
    newCover.emplace_back(cover[i]);
    newCounts.emplace_back(counts[i]);

    Gecode::IntVarArgs sameCounts;
    sameCounts << intVar(counts[i]);
    for (size_t j = i + 1; j < cover.size(); j++) {
      if (removed[j]) {
        continue;
      }
      if (cover[i] == cover[j]) {
        removed[j] = true;
        sameCounts << intVar(counts[j]);
      }
    }
    if (sameCounts.size() > 1) {
      Gecode::Region re;
      auto* sameCountDomains =
          re.alloc<Gecode::IntVarRanges>(sameCounts.size());
      for (int j = sameCounts.size(); j--;) {
        sameCountDomains[j].init(sameCounts[j]);
      }
      Gecode::Iter::Ranges::NaryInter domIntersection(re, sameCountDomains,
                                                      sameCounts.size());
      Gecode::IntSet newDom(domIntersection);
      if (newDom.size() == 0) {
        if (isFixedTo(reified, true)) {
          throw InconsistencyException("UNSAT");
        }
        if (!isFixedTo(reified, false)) {
          Gecode::rel(_space, boolVar(std::get<ConstraintVarId>(reified)),
                      Gecode::IRT_EQ, 0);
        }
        return {{}, {}};
      }
      Gecode::dom(_space, sameCounts, newDom);
    }
  }

  return std::pair<std::vector<Int>, std::vector<ConstraintVarId>>{newCover,
                                                                   newCounts};
}

std::pair<std::vector<Int>, std::pair<std::vector<Int>, std::vector<Int>>>
GecodeSolver::gcc_combine_covers(const std::vector<Int>& cover,
                                 const std::vector<Int>& lowerBounds,
                                 const std::vector<Int>& upperBounds) {
  std::vector<Int> newCover;
  newCover.reserve(cover.size());
  std::vector<Int> newLowerBounds;
  newLowerBounds.reserve(cover.size());
  std::vector<Int> newUpperBounds;
  newLowerBounds.reserve(cover.size());

  std::vector<bool> removed(cover.size(), false);
  for (size_t i = 0; i < cover.size(); i++) {
    if (removed[i]) {
      continue;
    }
    newCover.emplace_back(cover[i]);
    newLowerBounds.emplace_back(lowerBounds[i]);
    newUpperBounds.emplace_back(upperBounds[i]);

    for (size_t j = i + 1; j < cover.size(); j++) {
      if (removed[j]) {
        continue;
      }
      if (cover[i] == cover[j]) {
        removed[j] = true;
        newLowerBounds.back() = std::max(newLowerBounds.back(), lowerBounds[j]);
        newUpperBounds.back() = std::min(newUpperBounds.back(), lowerBounds[j]);
      }
    }
  }
  return std::pair<std::vector<Int>,
                   std::pair<std::vector<Int>, std::vector<Int>>>{
      newCover, {newLowerBounds, newUpperBounds}};
}

void GecodeSolver::gcc(const std::vector<ConstraintVarId>& inputs,
                       const std::vector<Int>& inputCover,
                       const std::vector<ConstraintVarId>& inputCounts,
                       const std::variant<bool, ConstraintVarId> reified) {
  const auto [cover, counts] =
      gcc_combine_covers(inputCover, inputCounts, reified);
  if ((cover.empty() && !inputCover.empty()) ||
      (counts.empty() && !inputCounts.empty())) {
    return;
  }

  if (gcc_sanity(inputs, cover, counts, reified)) {
    return;
  }
  Gecode::IntVarArgs inputVars = intVarArgs(inputs);
  Gecode::IntVarArgs countVars = intVarArgs(counts);
  const auto intArgCover = gcc_get_cover(inputVars, cover, countVars);
  unshare(_space, inputVars);
  if (isFixedTo(reified, true)) {
    Gecode::count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
    return;
  }
  Gecode::IntVarArgs reifiedCounts(intArgCover.size());
  for (int i = 0; i < countVars.size(); i++) {
    reifiedCounts[i] =
        Gecode::IntVar(_space, 0, static_cast<int>(inputVars.size()));
  }
  Gecode::BoolVarArgs reifiedEqualities(countVars.size());
  for (int i = 0; i < static_cast<int>(cover.size()); i++) {
    reifiedEqualities[i] = Gecode::BoolVar(_space, 0, 1);
    rel(_space, countVars[i], Gecode::IRT_EQ, reifiedCounts[i],
        reifiedEqualities[i]);
  }
  Gecode::count(_space, inputVars, reifiedCounts, intArgCover, Gecode::IPL_BND);
  if (std::holds_alternative<bool>(reified)) {
    assert(!std::get<bool>(reified));
    rel(_space, Gecode::BOT_AND, reifiedEqualities, 0);
  } else {
    rel(_space, Gecode::BOT_AND, reifiedEqualities,
        boolVar(std::get<ConstraintVarId>(reified)));
  }
}

void GecodeSolver::gcc_closed(
    const std::vector<ConstraintVarId>& inputs,
    const std::vector<Int>& inputCover,
    const std::vector<ConstraintVarId>& inputCounts,
    const std::variant<bool, ConstraintVarId> reified) {
  const auto [cover, counts] =
      gcc_combine_covers(inputCover, inputCounts, reified);

  if ((cover.empty() && !inputCover.empty()) ||
      (counts.empty() && !inputCounts.empty())) {
    return;
  }

  if (gcc_sanity(inputs, cover, counts, reified)) {
    return;
  }
  if (gcc_closed_sanity(cover, reified)) {
    return;
  }
  if (isFixedTo(reified, true)) {
    auto inputVars = intVarArgs(inputs);
    const auto intArgCover = intArgs(cover);
    const auto countVars = intVarArgs(counts);
    unshare(_space, inputVars);
    count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
    return;
  }
  Gecode::IntVarArgs inputVars = intVarArgs(inputs);
  Gecode::IntVarArgs countVars = intVarArgs(counts);
  auto intArgCover = gcc_get_cover(inputVars, cover, countVars);

  unshare(_space, inputVars);
  Gecode::IntVarArgs reifiedCounts(intArgCover.size());
  for (int i = 0; i < intArgCover.size(); i++) {
    reifiedCounts[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }

  Gecode::BoolVarArgs reifiedEqualities(countVars.size());
  for (int i = 0; i < reifiedEqualities.size(); i++) {
    reifiedEqualities[i] = Gecode::BoolVar(_space, 0, 1);
    if (i < static_cast<int>(cover.size())) {
      rel(_space, countVars[i], Gecode::IRT_EQ, reifiedCounts[i],
          reifiedEqualities[i]);
    } else {
      rel(_space, reifiedCounts[i], Gecode::IRT_EQ, 0, reifiedEqualities[i]);
    }
  }
  Gecode::count(_space, inputVars, reifiedCounts, intArgCover, Gecode::IPL_BND);
  if (std::holds_alternative<bool>(reified)) {
    assert(!std::get<bool>(reified));
    rel(_space, Gecode::BOT_AND, reifiedEqualities, 0);
  } else {
    rel(_space, Gecode::BOT_AND, reifiedEqualities,
        boolVar(std::get<ConstraintVarId>(reified)));
  }
}

void GecodeSolver::gcc_low_up(
    const std::vector<ConstraintVarId>& inputs,
    const std::vector<Int>& inputCover,
    const std::vector<Int>& inputLowerBounds,
    const std::vector<Int>& inputUpperBounds,
    const std::variant<bool, ConstraintVarId> reified) {
  const auto [cover, pair] =
      gcc_combine_covers(inputCover, inputLowerBounds, inputUpperBounds);
  const auto [lowerBounds, upperBounds] = pair;

  if (gcc_sanity(inputs, cover, lowerBounds, upperBounds, reified)) {
    return;
  }
  auto inputVars = intVarArgs(inputs);
  if (isFixedTo(reified, true)) {
    auto intArgCover = intArgs(cover);
    const auto lbound = intSharedArray(lowerBounds);
    const auto ubound = intSharedArray(upperBounds);
    Gecode::IntSetArgs counts(intArgCover.size());
    for (int i = intArgCover.size(); i--;) {
      counts[i] = Gecode::IntSet(lbound[i], ubound[i]);
    }

    const Gecode::IntSet cover_s(intArgCover);
    Gecode::Region re;
    auto* xrs = re.alloc<Gecode::IntVarRanges>(inputVars.size());
    for (int i = inputVars.size(); i--;) {
      xrs[i].init(inputVars[i]);
    }
    Gecode::Iter::Ranges::NaryUnion u(re, xrs, inputVars.size());
    Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::NaryUnion> uv(u);
    for (; uv(); ++uv) {
      if (!cover_s.in(uv.val())) {
        intArgCover << uv.val();
        counts << Gecode::IntSet(0, inputVars.size());
      }
    }
    Gecode::count(_space, inputVars, counts, intArgCover);
    return;
  }
  Gecode::IntVarArgs countVars(static_cast<int>(cover.size()));
  for (int i = 0; i < countVars.size(); i++) {
    countVars[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }
  const auto intArgCover = gcc_get_cover(inputVars, cover, countVars);

  unshare(_space, inputVars);

  Gecode::BoolVarArgs reifiedEqualities(static_cast<int>(cover.size()));
  for (int i = 0; i < reifiedEqualities.size(); i++) {
    reifiedEqualities[i] = Gecode::BoolVar(_space, 0, 1);
    dom(_space, countVars[i], static_cast<int>(lowerBounds[i]),
        static_cast<int>(upperBounds[i]), reifiedEqualities[i]);
  }
  Gecode::count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
  if (std::holds_alternative<bool>(reified)) {
    assert(!std::get<bool>(reified));
    rel(_space, Gecode::BOT_AND, reifiedEqualities, 0);
  } else {
    rel(_space, Gecode::BOT_AND, reifiedEqualities,
        boolVar(std::get<ConstraintVarId>(reified)));
  }
}

void GecodeSolver::gcc_low_up_closed(
    const std::vector<ConstraintVarId>& inputs,
    const std::vector<Int>& inputCover,
    const std::vector<Int>& inputLowerBounds,
    const std::vector<Int>& inputUpperBounds,
    const std::variant<bool, ConstraintVarId> reified) {
  const auto [cover, pair] =
      gcc_combine_covers(inputCover, inputLowerBounds, inputUpperBounds);
  const auto [lowerBounds, upperBounds] = pair;

  if (gcc_sanity(inputs, cover, lowerBounds, upperBounds, reified)) {
    return;
  }
  if (gcc_closed_sanity(cover, reified)) {
    return;
  }
  auto inputVars = intVarArgs(inputs);

  if (isFixedTo(reified, true)) {
    auto intArgCover = intArgs(cover);
    const auto lbound = intArgs(lowerBounds);
    const auto ubound = intArgs(upperBounds);
    Gecode::IntSetArgs countBounds(intArgCover.size());
    for (int i = intArgCover.size(); i--;)
      countBounds[i] = Gecode::IntSet(lbound[i], ubound[i]);
    unshare(_space, inputVars);
    count(_space, inputVars, countBounds, intArgCover, Gecode::IPL_BND);
    return;
  }

  Gecode::IntVarArgs countVars(static_cast<int>(cover.size()));
  for (int i = 0; i < countVars.size(); i++) {
    countVars[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }
  const auto intArgCover = gcc_get_cover(inputVars, cover, countVars);

  unshare(_space, inputVars);

  Gecode::BoolVarArgs reifiedEqualities(countVars.size());
  for (int i = 0; i < reifiedEqualities.size(); i++) {
    reifiedEqualities[i] = Gecode::BoolVar(_space, 0, 1);
    if (i < static_cast<int>(cover.size())) {
      dom(_space, countVars[i], static_cast<int>(lowerBounds[i]),
          static_cast<int>(upperBounds[i]), reifiedEqualities[i]);
    } else {
      rel(_space, countVars[i], Gecode::IRT_EQ, 0, reifiedEqualities[i]);
    }
  }
  Gecode::count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
  if (std::holds_alternative<bool>(reified)) {
    assert(!std::get<bool>(reified));
    rel(_space, Gecode::BOT_AND, reifiedEqualities, 0);
  } else {
    rel(_space, Gecode::BOT_AND, reifiedEqualities,
        boolVar(std::get<ConstraintVarId>(reified)));
  }
}

GecodeSolver::GecodeSolver() = default;

size_t GecodeSolver::numIntVars() const { return _space._iv.size(); }

size_t GecodeSolver::numBoolVars() const { return _space._bv.size(); }

ConstraintVarId GecodeSolver::newIntVar(const Int value) {
  const size_t ret = _space._iv.size();
  _space._iv.emplace_back(_space, value, value);
  return {ret, true};
}

ConstraintVarId GecodeSolver::newIntVar(const SearchDomain& dom) {
  const size_t ret = _space._iv.size();
  if (dom.isInterval()) {
    _space._iv.emplace_back(_space, dom.lowerBound(), dom.upperBound());
  } else {
    std::vector<int> values(dom.size());
    size_t i = 0;
    for (auto iter = dom.begin(); iter != dom.end(); ++iter) {
      values[i++] = static_cast<int>(*iter);
    }
    _space._iv.emplace_back(
        _space, Gecode::IntSet(values.data(), static_cast<int>(values.size())));
  }
  return {ret, true};
}

ConstraintVarId GecodeSolver::newBoolVar(const bool value) {
  const size_t ret = _space._bv.size();
  _space._bv.emplace_back(_space, value ? 1 : 0, value ? 1 : 0);
  return {ret, false};
}

ConstraintVarId GecodeSolver::newBoolVar() {
  const size_t ret = _space._bv.size();
  _space._bv.emplace_back(_space, 0, 1);
  return {ret, false};
}

SearchDomain GecodeSolver::intVarDomain(const ConstraintVarId varId) const {
  assert(varId.isIntVar());
  if (_space._iv[size_t{varId}].range()) {
    return SearchDomain(_space._iv[size_t{varId}].min(),
                        _space._iv[size_t{varId}].max());
  }
  std::vector<Int> values(_space._iv[size_t{varId}].size());
  size_t i = 0;
  for (Gecode::IntVarValues vals(_space._iv[size_t{varId}]); vals(); ++vals) {
    values[i++] = vals.val();
  }
  return SearchDomain(std::move(values));
}

SearchDomain GecodeSolver::boolVarDomain(const ConstraintVarId varId) const {
  assert(varId.isBoolVar());
  if (_space._bv[size_t{varId}].assigned()) {
    const Int viol = _space._bv[size_t{varId}].val() == 0 ? 1 : 0;
    return SearchDomain(viol, viol);
  }
  return SearchDomain(0, 1);
}

SearchDomain GecodeSolver::varDomain(const ConstraintVarId varId) const {
  return varId.isIntVar() ? intVarDomain(varId) : boolVarDomain(varId);
}

void GecodeSolver::fixPoint() {
  const Gecode::SpaceStatus s = _space.status();
  if (s == Gecode::SS_FAILED) {
    throw InconsistencyException("UNSAT");
  }
  assert(s == Gecode::SS_SOLVED);
}

void GecodeSolver::fix_bool(const ConstraintVarId var, const bool val) {
  Gecode::rel(_space, boolVar(var), Gecode::IRT_EQ, val ? 1 : 0);
}
void GecodeSolver::fix_int(const ConstraintVarId var, const Int val) {
  Gecode::rel(_space, boolVar(var), Gecode::IRT_EQ, static_cast<int>(val));
}

void GecodeSolver::array_bool_and(const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return array_bool_and(inputs, boolVar(reified).val() == 1);
  }
  array_bool_op(inputs, reified, Gecode::BOT_AND);
}

void GecodeSolver::array_bool_and(const std::vector<ConstraintVarId>& inputs,
                                  const bool shouldHold) {
  array_bool_op(inputs, shouldHold, Gecode::BOT_AND);
}

void GecodeSolver::array_bool_element(const ConstraintVarId index,
                                      const std::vector<bool>& parameters,
                                      const ConstraintVarId output,
                                      const Int offset) {
  const auto indexVar =
      Gecode::expr(_space, intVar(index) - static_cast<int>(offset));
  Gecode::element(_space, intSharedArray(parameters), indexVar,
                  boolVar(output));
}

void GecodeSolver::array_bool_element2d(
    const ConstraintVarId rowIndex, const ConstraintVarId colIndex,
    const std::vector<std::vector<bool>>& parameters,
    const ConstraintVarId output, const Int rowOffset, const Int colOffset) {
  Gecode::element(_space, intSharedArray(parameters), intVar(colIndex),
                  -static_cast<int>(colOffset),
                  static_cast<int>(parameters.front().size()), intVar(rowIndex),
                  -static_cast<int>(rowOffset),
                  static_cast<int>(parameters.size()), boolVar(output));
}

void GecodeSolver::array_bool_or(const std::vector<ConstraintVarId>& inputs,
                                 const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return array_bool_or(inputs, boolVar(reified).val() == 1);
  }
  array_bool_op(inputs, reified, Gecode::BOT_OR);
}

void GecodeSolver::array_bool_or(const std::vector<ConstraintVarId>& inputs,
                                 const bool shouldHold) {
  array_bool_op(inputs, shouldHold, Gecode::BOT_OR);
}

void GecodeSolver::array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId reified) {
  array_bool_op(inputs, reified, Gecode::BOT_XOR);
}

void GecodeSolver::array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                                  const bool shouldHold) {
  array_bool_op(inputs, shouldHold, Gecode::BOT_XOR);
}

void GecodeSolver::bool2int(const ConstraintVarId boolVarId,
                            const ConstraintVarId intVarId) {
  channel(_space, boolVar(boolVarId), intVar(intVarId));
}

void GecodeSolver::array_int_element(const ConstraintVarId index,
                                     const std::vector<Int>& parameters,
                                     const ConstraintVarId output,
                                     const Int offset) {
  const auto indexVar =
      Gecode::expr(_space, intVar(index) - static_cast<int>(offset));
  Gecode::element(_space, intSharedArray(parameters), indexVar, intVar(output));
}
void GecodeSolver::array_int_element2d(
    const ConstraintVarId rowIndex, const ConstraintVarId colIndex,
    const std::vector<std::vector<Int>>& parameters,
    const ConstraintVarId output, const Int rowOffset, const Int colOffset) {
  const auto rowIndexVar =
      Gecode::expr(_space, intVar(rowIndex) - static_cast<int>(rowOffset));
  const auto colIndexVar =
      Gecode::expr(_space, intVar(colIndex) - static_cast<int>(colOffset));
  Gecode::element(_space, intSharedArray(parameters), colIndexVar,
                  static_cast<int>(parameters.front().size()), rowIndexVar,
                  static_cast<int>(parameters.size()), intVar(output),
                  Gecode::IPL_DOM);
}

void GecodeSolver::array_int_maximum(const std::vector<ConstraintVarId>& inputs,
                                     const ConstraintVarId output) {
  max(_space, intVarArgs(inputs), intVar(output), Gecode::IPL_BND);
}

void GecodeSolver::array_int_minimum(const std::vector<ConstraintVarId>& inputs,
                                     const ConstraintVarId output) {
  min(_space, intVarArgs(inputs), intVar(output), Gecode::IPL_BND);
}

void GecodeSolver::array_var_bool_element(
    const ConstraintVarId index, const std::vector<ConstraintVarId>& inputs,
    const ConstraintVarId output, const Int offset) {
  const bool allParams = std::ranges::all_of(
      inputs,
      [&](const ConstraintVarId varId) { return boolVar(varId).assigned(); });
  if (allParams) {
    std::vector<bool> params(inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
      params[i] = boolVar(inputs[i]).val() == 1;
    }
    return array_bool_element(index, params, output, offset);
  }
  Gecode::element(_space, boolVarArgs(inputs), intVar(index),
                  -static_cast<int>(offset), boolVar(output), Gecode::IPL_DOM);
}

void GecodeSolver::array_var_bool_element2d(
    const ConstraintVarId rowIndex, const ConstraintVarId colIndex,
    const std::vector<std::vector<ConstraintVarId>>& inputs,
    const ConstraintVarId output, const Int rowOffset, const Int colOffset) {
  const bool allParams =
      std::ranges::all_of(inputs, [&](const std::vector<ConstraintVarId>& row) {
        return std::ranges::all_of(row, [&](const ConstraintVarId varId) {
          return boolVar(varId).assigned();
        });
      });
  if (allParams) {
    std::vector<std::vector<bool>> params(
        inputs.size(), std::vector<bool>(inputs.front().size()));
    for (size_t r = 0; r < inputs.size(); ++r) {
      for (size_t c = 0; c < inputs.front().size(); ++c) {
        params[r][c] = boolVar(inputs[r][c]).val() == 1;
      }
    }
    array_bool_element2d(rowIndex, colIndex, params, output, rowOffset,
                         colOffset);
    return;
  }
  Gecode::element(_space, boolVarArgs(inputs), intVar(colIndex),
                  -static_cast<int>(colOffset),
                  static_cast<int>(inputs.front().size()), intVar(rowIndex),
                  -static_cast<int>(rowOffset), static_cast<int>(inputs.size()),
                  boolVar(output));
}

void GecodeSolver::array_var_int_element(
    const ConstraintVarId index, const std::vector<ConstraintVarId>& inputs,
    const ConstraintVarId output, const Int offset) {
  const bool allParams = std::ranges::all_of(
      inputs,
      [&](const ConstraintVarId varId) { return intVar(varId).assigned(); });
  if (allParams) {
    std::vector<Int> params(inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
      params[i] = static_cast<Int>(intVar(inputs[i]).val());
    }
    array_int_element(index, params, output, offset);
    return;
  }
  const auto indexVar =
      Gecode::expr(_space, intVar(index) - static_cast<int>(offset));
  Gecode::element(_space, intVarArgs(inputs), indexVar, intVar(output));
}

void GecodeSolver::array_var_int_element2d(
    const ConstraintVarId rowIndex, const ConstraintVarId colIndex,
    const std::vector<std::vector<ConstraintVarId>>& inputs,
    const ConstraintVarId output, const Int rowOffset, const Int colOffset) {
  const bool allParams =
      std::ranges::all_of(inputs, [&](const std::vector<ConstraintVarId>& row) {
        return std::ranges::all_of(row, [&](const ConstraintVarId varId) {
          return intVar(varId).assigned();
        });
      });
  if (allParams) {
    std::vector<std::vector<Int>> params(
        inputs.size(), std::vector<Int>(inputs.front().size()));
    for (size_t r = 0; r < inputs.size(); ++r) {
      for (size_t c = 0; c < inputs.front().size(); ++c) {
        params[r][c] = static_cast<Int>(intVar(inputs[r][c]).val());
      }
    }
    array_int_element2d(rowIndex, colIndex, params, output, rowOffset,
                        colOffset);
    return;
  }
  const auto rowIndexVar =
      Gecode::expr(_space, intVar(rowIndex) - static_cast<int>(rowOffset));
  const auto colIndexVar =
      Gecode::expr(_space, intVar(colIndex) - static_cast<int>(colOffset));
  Gecode::element(_space, intVarArgs(inputs), colIndexVar,
                  static_cast<int>(inputs.front().size()), rowIndexVar,
                  static_cast<int>(inputs.size()), intVar(output));
}

void GecodeSolver::bool_and(const ConstraintVarId b1, const ConstraintVarId b2,
                            const bool shouldHold) {
  bool_op(b1, b2, shouldHold, Gecode::BOT_AND);
}

void GecodeSolver::bool_and_reif(const ConstraintVarId b1,
                                 const ConstraintVarId b2,
                                 const ConstraintVarId reified) {
  bool_op(b1, b2, reified, Gecode::BOT_AND);
}

void GecodeSolver::bool_clause(const std::vector<ConstraintVarId>& posInputs,
                               const std::vector<ConstraintVarId>& negInputs,
                               const bool shouldHold) {
  clause(_space, Gecode::BoolOpType::BOT_OR, boolVarArgs(posInputs),
         boolVarArgs(negInputs), shouldHold ? 1 : 0, Gecode::IPL_BND);
}

void GecodeSolver::bool_clause_reif(
    const std::vector<ConstraintVarId>& posInputs,
    const std::vector<ConstraintVarId>& negInputs,
    const ConstraintVarId reified) {
  clause(_space, Gecode::BoolOpType::BOT_OR, boolVarArgs(posInputs),
         boolVarArgs(negInputs), boolVar(reified), Gecode::IPL_BND);
}

void GecodeSolver::bool_rel_reif(const ConstraintVarId lhs,
                                 const RelationType relation,
                                 const ConstraintVarId rhs,
                                 const ConstraintVarId reified) {
  Gecode::rel(_space, boolVar(lhs), toGecodeIntRelType(relation), boolVar(rhs),
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::bool_rel(const ConstraintVarId lhs,
                            const RelationType relation,
                            const ConstraintVarId rhs, const bool shouldHold) {
  Gecode::rel(_space, boolVar(lhs), toGecodeIntRelType(relation, shouldHold),
              boolVar(rhs), Gecode::IPL_BND);
}

void GecodeSolver::int_rel_reif(const ConstraintVarId lhs,
                                const RelationType relation,
                                const ConstraintVarId rhs,
                                const ConstraintVarId reified) {
  Gecode::rel(_space, intVar(lhs), toGecodeIntRelType(relation), intVar(rhs),
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::int_rel_reif(const ConstraintVarId lhs,
                                const RelationType relation, const Int rhs,
                                const ConstraintVarId reified) {
  Gecode::rel(_space, intVar(lhs), toGecodeIntRelType(relation),
              static_cast<int>(rhs),
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::int_rel(const ConstraintVarId lhs,
                           const RelationType relation,
                           const ConstraintVarId rhs, const bool shouldHold) {
  Gecode::rel(_space, intVar(lhs), toGecodeIntRelType(relation, shouldHold),
              intVar(rhs), Gecode::IPL_BND);
}
void GecodeSolver::bool_eq(const ConstraintVarId b1, const ConstraintVarId b2,
                           const bool shouldHold) {
  bool_rel(b1, RelationType::REL_TYPE_EQ, b2, shouldHold);
}

void GecodeSolver::bool_eq_reif(const ConstraintVarId b1,
                                const ConstraintVarId b2,
                                const ConstraintVarId reified) {
  bool_rel_reif(b1, RelationType::REL_TYPE_EQ, b2, reified);
}

void GecodeSolver::bool_le(const ConstraintVarId b1, const ConstraintVarId b2,
                           const bool shouldHold) {
  bool_rel(b1, RelationType::REL_TYPE_LE, b2, shouldHold);
}

void GecodeSolver::bool_le_reif(const ConstraintVarId b1,
                                const ConstraintVarId b2,
                                const ConstraintVarId reified) {
  bool_rel_reif(b1, RelationType::REL_TYPE_LE, b2, reified);
}

void GecodeSolver::bool_lin_eq(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const ConstraintVarId rhs, const Int rhsOffset) {
  if (rhsOffset == 0) {
    return linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
                  Gecode::IRT_EQ, intVar(rhs));
  }
  const auto rhsVar = expr(_space, intVar(rhs) + static_cast<int>(rhsOffset));
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs), Gecode::IRT_EQ,
         rhsVar);
}

void GecodeSolver::bool_lin(const std::vector<Int>& coeffs,
                            const std::vector<ConstraintVarId>& inputs,
                            const RelationType relation, const Int rhs,
                            const bool shouldHold) {
  bool_lin_rel(coeffs, inputs, relation, rhs, shouldHold);
}

void GecodeSolver::bool_lin_reif(const std::vector<Int>& coeffs,
                                 const std::vector<ConstraintVarId>& inputs,
                                 const RelationType relation, const Int rhs,
                                 const ConstraintVarId reified) {
  bool_lin_rel(coeffs, inputs, relation, rhs, reified);
}

void GecodeSolver::bool_lt(const ConstraintVarId b1, const ConstraintVarId b2,
                           const bool shouldHold) {
  bool_rel(b1, RelationType::REL_TYPE_LE, b2, shouldHold);
}

void GecodeSolver::bool_lt_reif(const ConstraintVarId b1,
                                const ConstraintVarId b2,
                                const ConstraintVarId reified) {
  bool_rel_reif(b1, RelationType::REL_TYPE_LE, b2, reified);
}

void GecodeSolver::bool_not(const ConstraintVarId b1, const ConstraintVarId b2,
                            const bool shouldHold) {
  bool_rel(b1, RelationType::REL_TYPE_NE, b2, shouldHold);
}

void GecodeSolver::bool_not_reif(const ConstraintVarId b1,
                                 const ConstraintVarId b2,
                                 const ConstraintVarId reified) {
  bool_rel_reif(b1, RelationType::REL_TYPE_NE, b2, reified);
}
void GecodeSolver::bool_or(const ConstraintVarId b1, const ConstraintVarId b2,
                           const bool shouldHold) {
  bool_op(b1, b2, shouldHold, Gecode::BOT_OR);
}
void GecodeSolver::bool_or_reif(const ConstraintVarId b1,
                                const ConstraintVarId b2,
                                const ConstraintVarId reified) {
  bool_op(b1, b2, reified, Gecode::BOT_OR);
}

void GecodeSolver::bool_xor(const ConstraintVarId b1, const ConstraintVarId b2,
                            const bool shouldHold) {
  bool_op(b1, b2, shouldHold, Gecode::BOT_OR);
}

void GecodeSolver::bool_xor_reif(const ConstraintVarId b1,
                                 const ConstraintVarId b2,
                                 const ConstraintVarId reified) {
  bool_op(b1, b2, reified, Gecode::BOT_OR);
}

void GecodeSolver::int_abs(const ConstraintVarId lhs,
                           const ConstraintVarId rhs) {
  abs(_space, intVar(lhs), intVar(rhs));
}

void GecodeSolver::int_div(const ConstraintVarId numerator,
                           const ConstraintVarId denominator,
                           const ConstraintVarId quotient) {
  Gecode::IntVarArgs arr{intVar(numerator), intVar(denominator),
                         intVar(quotient)};
  unshare(_space, arr);
  const Gecode::BoolVar numeratorIsZero(_space, 0, 1);
  Gecode::rel(_space, arr[0], Gecode::IRT_EQ, 0, numeratorIsZero);
  Gecode::rel(_space, arr[2], Gecode::IRT_EQ, 0,
              Gecode::Reify(numeratorIsZero, Gecode::RM_IMP));
  const Gecode::BoolVar denominatorIsOne(_space, 0, 1);
  Gecode::rel(_space, arr[1], Gecode::IRT_EQ, 1, denominatorIsOne);
  Gecode::rel(_space, arr[0], Gecode::IRT_EQ, arr[2],
              Gecode::Reify(denominatorIsOne, Gecode::RM_IMP));
  const Gecode::BoolVar denominatorIsNegOne(_space, 0, 1);
  Gecode::rel(_space, arr[1], Gecode::IRT_EQ, 1, denominatorIsNegOne);
  Gecode::rel(_space, arr[0], Gecode::IRT_EQ, expr(_space, -arr[2]),
              Gecode::Reify(denominatorIsNegOne, Gecode::RM_IMP));
  Gecode::div(_space, arr[0], arr[1], arr[2]);
}

void GecodeSolver::int_eq(const ConstraintVarId lhs, const ConstraintVarId rhs,
                          const bool shouldHold) {
  int_rel(lhs, RelationType::REL_TYPE_EQ, rhs, shouldHold);
}

void GecodeSolver::int_eq_reif(const ConstraintVarId lhs,
                               const ConstraintVarId rhs,
                               const ConstraintVarId reified) {
  int_rel_reif(lhs, RelationType::REL_TYPE_EQ, rhs, reified);
}

void GecodeSolver::int_eq_reif(const ConstraintVarId lhs, const Int rhs,
                               const ConstraintVarId reified) {
  int_rel_reif(lhs, RelationType::REL_TYPE_EQ, rhs, reified);
}

void GecodeSolver::int_le(const ConstraintVarId lhs, const ConstraintVarId rhs,
                          const bool shouldHold) {
  int_rel(lhs, RelationType::REL_TYPE_LE, rhs, shouldHold);
}

void GecodeSolver::int_le_reif(const ConstraintVarId lhs, const Int rhs,
                               const ConstraintVarId reified) {
  int_rel_reif(lhs, RelationType::REL_TYPE_LE, rhs, reified);
}

void GecodeSolver::int_le_reif(const ConstraintVarId lhs,
                               const ConstraintVarId rhs,
                               const ConstraintVarId reified) {
  int_rel_reif(lhs, RelationType::REL_TYPE_LE, rhs, reified);
}

void GecodeSolver::int_lin_eq(const std::vector<Int>& coeffs,
                              const std::vector<ConstraintVarId>& inputs,
                              const ConstraintVarId rhs, const Int rhsOffset) {
  int_lin_rel(coeffs, inputs, RelationType::REL_TYPE_EQ, rhs, rhsOffset);
}

void GecodeSolver::int_lin(const std::vector<Int>& coeffs,
                           const std::vector<ConstraintVarId>& inputs,
                           const RelationType relation, const Int rhs,
                           const bool shouldHold) {
  int_lin_rel(coeffs, inputs, relation, rhs, shouldHold);
}

void GecodeSolver::int_lin_reif(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const RelationType relation, const Int rhs,
                                const ConstraintVarId reified) {
  int_lin_rel(coeffs, inputs, relation, rhs, reified);
}

void GecodeSolver::int_lt(const ConstraintVarId lhs, const ConstraintVarId rhs,
                          const bool shouldHold) {
  int_rel(lhs, RelationType::REL_TYPE_LT, rhs, shouldHold);
}

void GecodeSolver::int_lt_reif(const ConstraintVarId lhs, const Int rhs,
                               const ConstraintVarId reified) {
  int_rel_reif(lhs, RelationType::REL_TYPE_LT, rhs, reified);
}

void GecodeSolver::int_lt_reif(const ConstraintVarId lhs,
                               const ConstraintVarId rhs,
                               const ConstraintVarId reified) {
  int_rel_reif(lhs, RelationType::REL_TYPE_LT, rhs, reified);
}

void GecodeSolver::int_max(const ConstraintVarId a, const ConstraintVarId b,
                           const ConstraintVarId maximum) {
  max(_space, intVar(a), intVar(b), intVar(maximum));
}

void GecodeSolver::int_min(const ConstraintVarId a, const ConstraintVarId b,
                           const ConstraintVarId minimum) {
  min(_space, intVar(a), intVar(b), intVar(minimum));
}

void GecodeSolver::int_mod(const ConstraintVarId numerator,
                           const ConstraintVarId denominator,
                           const ConstraintVarId remainder) {
  Gecode::IntVarArgs arr{intVar(numerator), intVar(denominator),
                         intVar(remainder)};
  unshare(_space, arr);
  mod(_space, arr[0], arr[1], arr[2]);
}
void GecodeSolver::int_ne(const ConstraintVarId lhs, const ConstraintVarId rhs,
                          const bool shouldHold) {
  int_rel(lhs, RelationType::REL_TYPE_NE, rhs, shouldHold);
}
void GecodeSolver::int_ne_reif(const ConstraintVarId lhs, const Int rhs,
                               const ConstraintVarId reified) {
  int_rel_reif(lhs, RelationType::REL_TYPE_NE, rhs, reified);
}

void GecodeSolver::int_ne_reif(const ConstraintVarId lhs,
                               const ConstraintVarId rhs,
                               const ConstraintVarId reified) {
  int_rel_reif(lhs, RelationType::REL_TYPE_NE, rhs, reified);
}

void GecodeSolver::int_plus(const ConstraintVarId a, const ConstraintVarId b,
                            const ConstraintVarId sum) {
  const auto aVar = intVar(a);
  const auto bVar = intVar(b);
  const auto sumVar = intVar(sum);
  if (aVar.assigned()) {
    Gecode::rel(_space, expr(_space, aVar.val() + bVar), Gecode::IRT_EQ,
                sumVar);
  } else if (bVar.assigned()) {
    Gecode::rel(_space, expr(_space, aVar + bVar.val()), Gecode::IRT_EQ,
                sumVar);
  } else if (sumVar.assigned()) {
    Gecode::rel(_space, expr(_space, aVar + bVar), Gecode::IRT_EQ,
                sumVar.val());
  } else {
    Gecode::rel(_space, expr(_space, aVar + bVar), Gecode::IRT_EQ, sumVar);
  }
}

void GecodeSolver::int_pow(const ConstraintVarId base,
                           const ConstraintVarId exponent,
                           const ConstraintVarId power) {
  const auto exponentVar = intVar(exponent);
  if (exponentVar.assigned()) {
    pow(_space, intVar(base), exponentVar.val(), intVar(power));
  }
}

void GecodeSolver::int_times(const ConstraintVarId a, const ConstraintVarId b,
                             const ConstraintVarId product) {
  mult(_space, intVar(a), intVar(b), intVar(product));
}
void GecodeSolver::fzn_all_different_int(
    const std::vector<ConstraintVarId>& inputs, const bool shouldHold) {
  if (shouldHold) {
    auto inputVars = intVarArgs(inputs);
    Gecode::unshare(_space, inputVars);  // Is this really needed?
    return Gecode::distinct(_space, inputVars);
  }
  Gecode::nvalues(_space, intVarArgs(inputs), Gecode::IRT_LE,
                  static_cast<int>(inputs.size()));
}
void GecodeSolver::fzn_all_different_int_reif(
    const std::vector<ConstraintVarId>& inputs, const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_all_different_int(inputs, boolVar(reified).val() == 1);
  }
  const Gecode::IntVar numVals(_space, 0, static_cast<int>(inputs.size()));
  Gecode::nvalues(_space, intVarArgs(inputs), Gecode::IRT_EQ, numVals);
  Gecode::rel(_space, numVals, Gecode::IRT_EQ, static_cast<int>(inputs.size()),
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV));
}

void GecodeSolver::fzn_all_equal_int(const std::vector<ConstraintVarId>& inputs,
                                     const bool shouldHold) {
  Gecode::rel(_space, intVarArgs(inputs),
              shouldHold ? Gecode::IRT_EQ : Gecode::IRT_NQ);
}

void GecodeSolver::fzn_all_equal_int_reif(
    const std::vector<ConstraintVarId>& inputs, const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_all_equal_int(inputs, boolVar(reified).val() == 1);
  }
  const Gecode::IntVar numVals(_space, 0, static_cast<int>(inputs.size()));
  Gecode::nvalues(_space, intVarArgs(inputs), Gecode::IRT_EQ, numVals);
  Gecode::rel(_space, numVals, Gecode::IRT_EQ, 1,
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV));
}

void GecodeSolver::fzn_circuit(const std::vector<ConstraintVarId>& inputs,
                               const Int offset, const bool shouldHold) {
  if (shouldHold) {
    auto inputVars = intVarArgs(inputs);
    unshare(_space, inputVars);
    return Gecode::circuit(_space, static_cast<int>(offset), inputVars);
  }
}

void GecodeSolver::fzn_circuit_reif(const std::vector<ConstraintVarId>& inputs,
                                    const Int offset,
                                    const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_circuit(inputs, offset, boolVar(reified).val() == 1);
  }
}

void GecodeSolver::fzn_global_cardinality(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const bool shouldHold) {
  return gcc(inputs, cover, counts, shouldHold);
}

void GecodeSolver::fzn_global_cardinality_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const ConstraintVarId reified) {
  return gcc(inputs, cover, counts, reified);
}
void GecodeSolver::fzn_global_cardinality_closed(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const bool shouldHold) {
  return gcc_closed(inputs, cover, counts, shouldHold);
}

void GecodeSolver::fzn_global_cardinality_closed_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const ConstraintVarId reified) {
  return gcc_closed(inputs, cover, counts, reified);
}

void GecodeSolver::fzn_global_cardinality_low_up(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const bool shouldHold) {
  return gcc_low_up(inputs, cover, lowerBounds, upperBounds, shouldHold);
}
void GecodeSolver::fzn_global_cardinality_low_up_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const ConstraintVarId reified) {
  return gcc_low_up(inputs, cover, lowerBounds, upperBounds, reified);
}

void GecodeSolver::fzn_global_cardinality_low_up_closed(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const bool shouldHold) {
  return gcc_low_up_closed(inputs, cover, lowerBounds, upperBounds, shouldHold);
}

void GecodeSolver::fzn_global_cardinality_low_up_closed_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const ConstraintVarId reified) {
  return gcc_low_up_closed(inputs, cover, lowerBounds, upperBounds, reified);
}

void GecodeSolver::fzn_count(const Int bound, const RelationType relation,
                             const std::vector<ConstraintVarId>& inputs,
                             const Int needle, const bool shouldHold) {
  Gecode::count(_space, intVarArgs(inputs), static_cast<int>(needle),
                toGecodeIntRelType(relationTypeConverse(relation), shouldHold),
                static_cast<int>(bound), Gecode::IPL_DOM);
}

void GecodeSolver::fzn_count(const ConstraintVarId bound,
                             const RelationType relation,
                             const std::vector<ConstraintVarId>& inputs,
                             const Int needle, const bool shouldHold) {
  if (intVar(bound).assigned()) {
    return fzn_count(intVar(bound).val(), relation, inputs, needle, shouldHold);
  }
  Gecode::count(_space, intVarArgs(inputs), static_cast<int>(needle),
                toGecodeIntRelType(relationTypeConverse(relation), shouldHold),
                intVar(bound), Gecode::IPL_DOM);
}

void GecodeSolver::fzn_count(const Int bound, const RelationType relation,
                             const std::vector<ConstraintVarId>& inputs,
                             const ConstraintVarId needle,
                             const bool shouldHold) {
  if (intVar(needle).assigned()) {
    return fzn_count(bound, relation, inputs, intVar(needle).val(), shouldHold);
  }
  const Gecode::IntPropLevel ipl = allFixed(inputs) || intVar(needle).size() < 4
                                       ? Gecode::IPL_DOM
                                       : Gecode::IPL_DEF;
  Gecode::count(_space, intVarArgs(inputs), intVar(needle),
                toGecodeIntRelType(relationTypeConverse(relation), shouldHold),
                static_cast<int>(bound), ipl);
}

void GecodeSolver::fzn_count(const ConstraintVarId bound,
                             const RelationType relation,
                             const std::vector<ConstraintVarId>& inputs,
                             const ConstraintVarId needle,
                             const bool shouldHold) {
  if (intVar(bound).assigned() && intVar(needle).assigned()) {
    return fzn_count(intVar(bound).val(), relation, inputs,
                     intVar(needle).val(), shouldHold);
  }
  if (intVar(bound).assigned()) {
    return fzn_count(intVar(bound).val(), relation, inputs, needle, shouldHold);
  }
  if (intVar(needle).assigned()) {
    return fzn_count(bound, relation, inputs, intVar(needle).val(), shouldHold);
  }
  const Gecode::IntPropLevel ipl =
      allFixed(inputs) || intVar(needle).size() < 4 || intVar(bound).size() < 4
          ? Gecode::IPL_DOM
          : Gecode::IPL_DEF;
  Gecode::count(_space, intVarArgs(inputs), intVar(needle),
                toGecodeIntRelType(relationTypeConverse(relation), shouldHold),
                intVar(bound), ipl);
}

void GecodeSolver::fzn_count_reif(const Int bound, const RelationType relation,
                                  const std::vector<ConstraintVarId>& inputs,
                                  const Int needle,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_count(bound, relation, inputs, needle,
                     boolVar(reified).val() == 1);
  }
  const Gecode::IntVar amount(_space, 0, Gecode::Int::Limits::max);
  count(_space, intVarArgs(inputs), static_cast<int>(needle), Gecode::IRT_EQ,
        amount);
  rel(_space, amount, toGecodeIntRelType(relationTypeConverse(relation)),
      static_cast<int>(bound), boolVar(reified));
}

void GecodeSolver::fzn_count_reif(const ConstraintVarId bound,
                                  const RelationType relation,
                                  const std::vector<ConstraintVarId>& inputs,
                                  const Int needle,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_count(bound, relation, inputs, needle,
                     boolVar(reified).val() == 1);
  }
  if (intVar(bound).assigned()) {
    return fzn_count_reif(intVar(bound).val(), relation, inputs, needle,
                          reified);
  }
  const Gecode::IntVar amount(_space, 0, Gecode::Int::Limits::max);
  count(_space, intVarArgs(inputs), static_cast<int>(needle), Gecode::IRT_EQ,
        amount);
  rel(_space, intVar(bound), toGecodeIntRelType(relation), amount,
      boolVar(reified));
}

void GecodeSolver::fzn_count_reif(const Int bound, const RelationType relation,
                                  const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId needle,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_count(bound, relation, inputs, needle,
                     boolVar(reified).val() == 1);
  }
  if (intVar(needle).assigned()) {
    return fzn_count_reif(bound, relation, inputs, intVar(needle).val(),
                          reified);
  }
  const Gecode::IntVar amount(_space, 0, Gecode::Int::Limits::max);
  count(_space, intVarArgs(inputs), intVar(needle), Gecode::IRT_EQ, amount);
  rel(_space, amount, toGecodeIntRelType(relationTypeConverse(relation)),
      static_cast<int>(bound), boolVar(reified));
}

void GecodeSolver::fzn_count_reif(const ConstraintVarId bound,
                                  const RelationType relation,
                                  const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId needle,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_count(bound, relation, inputs, needle,
                     boolVar(reified).val() == 1);
  }
  if (intVar(bound).assigned() && intVar(needle).assigned()) {
    return fzn_count_reif(intVar(bound).val(), relation, inputs,
                          intVar(needle).val(), reified);
  }
  if (intVar(bound).assigned()) {
    return fzn_count_reif(intVar(bound).val(), relation, inputs, needle,
                          reified);
  }
  if (intVar(needle).assigned()) {
    return fzn_count_reif(bound, relation, inputs, intVar(needle).val(),
                          reified);
  }
  const Gecode::IntVar amount(_space, 0, Gecode::Int::Limits::max);
  count(_space, intVarArgs(inputs), intVar(needle), Gecode::IRT_EQ, amount);
  rel(_space, intVar(bound), toGecodeIntRelType(relation), amount,
      boolVar(reified));
}

void GecodeSolver::fzn_table_bool(const std::vector<ConstraintVarId>& inputs,
                                  const std::vector<std::vector<bool>>& table,
                                  const bool shouldHold) {
  auto inputVars = boolVarArgs(inputs);
  const Gecode::TupleSet ts = tupleSet(table);
  Gecode::unshare(_space, inputVars);
  extensional(_space, inputVars, ts, shouldHold);
}

void GecodeSolver::fzn_table_bool_reif(
    const std::vector<ConstraintVarId>& inputs,
    const std::vector<std::vector<Int>>& table, const ConstraintVarId reified) {
  auto inputVars = boolVarArgs(inputs);
  const Gecode::TupleSet ts = tupleSet(table);
  Gecode::unshare(_space, inputVars);
  extensional(_space, inputVars, ts,
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV));
}

void GecodeSolver::fzn_table_int(const std::vector<ConstraintVarId>& inputs,
                                 const std::vector<std::vector<Int>>& table,
                                 const bool shouldHold) {
  auto inputVars = intVarArgs(inputs);
  const Gecode::TupleSet ts = tupleSet(table);
  Gecode::unshare(_space, inputVars);
  extensional(_space, inputVars, ts, shouldHold);
}

void GecodeSolver::fzn_table_int_reif(
    const std::vector<ConstraintVarId>& inputs,
    const std::vector<std::vector<Int>>& table, const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_table_int(inputs, table, boolVar(reified).val() == 1);
  }
  auto inputVars = intVarArgs(inputs);
  const Gecode::TupleSet ts = tupleSet(table);
  Gecode::unshare(_space, inputVars);
  extensional(_space, inputVars, ts,
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV));
}

void GecodeSolver::set_in(const ConstraintVarId varId,
                          const SortedUniqueVector& values,
                          const bool shouldHold) {
  if (values->empty()) {
    if (shouldHold) {
      throw InconsistencyException("UNSAT");
    }
    return;
  }
  if (shouldHold) {
    Gecode::dom(_space, intVar(varId), intSet(values));
  }
  for (const auto val : *values) {
    Gecode::rel(_space, intVar(varId), Gecode::IRT_NQ, static_cast<int>(val));
  }
}

void GecodeSolver::set_in_reif(const ConstraintVarId varId,
                               const SortedUniqueVector& values,
                               const ConstraintVarId reified) {
  if (values->empty()) {
    Gecode::rel(_space, boolVar(reified), Gecode::IRT_EQ, 0);
    return;
  }
  Gecode::dom(_space, intVar(varId), intSet(values), boolVar(reified));
}

void GecodeSolver::nvalue(const ConstraintVarId numVals,
                          const std::vector<ConstraintVarId>& inputs) {
  const auto inputVars = intVarArgs(inputs);
  if (intVar(numVals).assigned()) {
    return Gecode::nvalues(_space, inputVars, Gecode::IRT_EQ,
                           intVar(numVals).val());
  }
  return Gecode::nvalues(_space, inputVars, Gecode::IRT_EQ, intVar(numVals));
}

void GecodeSolver::nvalue_lt(const Int numVals,
                             const std::vector<ConstraintVarId>& inputs) {
  return Gecode::nvalues(_space, intVarArgs(inputs), Gecode::IRT_LE,
                         static_cast<int>(numVals));
}

}  // namespace atlantis::invariantgraph
