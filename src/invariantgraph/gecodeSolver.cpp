#include "atlantis/invariantgraph/gecodeSolver.hpp"

#include <gecode/minimodel.hh>
#include <numeric>
#include <ranges>
#include <vector>

#include "atlantis/exceptions/exceptions.hpp"

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
  Gecode::TupleSet ts(static_cast<int>(table.size()));
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
  Gecode::TupleSet ts(static_cast<int>(table.size()));
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
  if ((*values).empty()) {
    return {};
  }
  if (values.isInterval()) {
    return Gecode::IntSet(static_cast<int>((*values).front()),
                          static_cast<int>((*values).back()));
  }
  Gecode::Region re;
  int* is = re.alloc<int>((*values).size());
  for (Int i = static_cast<Int>((*values).size()) - 1; i >= 0; --i) {
    is[i] = static_cast<int>((*values)[i]);
  }
  return Gecode::IntSet(is, static_cast<int>((*values).size()));
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

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const Int rhs, const ConstraintVarId reified,
                                const RelationType relation) {
  const auto& reif = boolVar(reified);
  if (reif.assigned()) {
    return bool_lin_rel(coeffs, inputs, rhs, reif.val() == 1, relation);
  }
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
         toGecodeIntRelType(relation), static_cast<int>(rhs),
         Gecode::Reify(reif, Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const ConstraintVarId rhs, const Int rhsOffset,
                                const RelationType relation) {
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
                                const Int rhs, const bool shouldHold,
                                const RelationType relation) {
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
         toGecodeIntRelType(relation, shouldHold), static_cast<int>(rhs));
}

void GecodeSolver::int_lin_rel(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const Int rhs, const ConstraintVarId reified,
                               const RelationType relation) {
  const auto& reif = boolVar(reified);
  if (reif.assigned()) {
    return int_lin_rel(coeffs, inputs, rhs, reif.val() == 1, relation);
  }
  linear(_space, intSharedArray(coeffs), intVarArgs(inputs),
         toGecodeIntRelType(relation), static_cast<int>(rhs),
         Gecode::Reify(reif, Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::int_lin_rel(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const ConstraintVarId rhs, const Int rhsOffset,
                               const RelationType relation) {
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
                               const Int rhs, const bool shouldHold,
                               const RelationType relation) {
  linear(_space, intSharedArray(coeffs), intVarArgs(inputs),
         toGecodeIntRelType(relation, shouldHold), static_cast<int>(rhs));
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
  Gecode::element(_space, boolVarArgs(inputs), intVar(colIndex), -colOffset,
                  static_cast<int>(inputs.front().size()), intVar(rowIndex),
                  -rowOffset, static_cast<int>(inputs.size()), boolVar(output));
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
  bool_lin_rel(coeffs, inputs, rhs, shouldHold, relation);
}

void GecodeSolver::bool_lin_reif(const std::vector<Int>& coeffs,
                                 const std::vector<ConstraintVarId>& inputs,
                                 const RelationType relation, const Int rhs,
                                 const ConstraintVarId reified) {
  bool_lin_rel(coeffs, inputs, rhs, reified, relation);
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
  Gecode::rel(_space, arr[2], Gecode::IRT_EQ, 0, Gecode::Reify(numeratorIsZero, Gecode::RM_IMP));
  const Gecode::BoolVar denominatorIsOne(_space, 0, 1);
  Gecode::rel(_space, arr[1], Gecode::IRT_EQ, 1, denominatorIsOne);
  Gecode::rel(_space, arr[0], Gecode::IRT_EQ, arr[2], Gecode::Reify(denominatorIsOne, Gecode::RM_IMP));
  const Gecode::BoolVar denominatorIsNegOne(_space, 0, 1);
  Gecode::rel(_space, arr[1], Gecode::IRT_EQ, 1, denominatorIsNegOne);
  Gecode::rel(_space, arr[0], Gecode::IRT_EQ, expr(_space, -arr[2]), Gecode::Reify(denominatorIsNegOne, Gecode::RM_IMP));
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
  int_lin_rel(coeffs, inputs, rhs, rhsOffset, RelationType::REL_TYPE_EQ);
}

void GecodeSolver::int_lin(const std::vector<Int>& coeffs,
                           const std::vector<ConstraintVarId>& inputs,
                           const RelationType relation, const Int rhs,
                           const bool shouldHold) {
  int_lin_rel(coeffs, inputs, rhs, shouldHold, relation);
}

void GecodeSolver::int_lin_reif(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const RelationType relation, const Int rhs,
                                const ConstraintVarId reified) {
  int_lin_rel(coeffs, inputs, rhs, reified, relation);
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
  Gecode::IntVarArgs inputVars = intVarArgs(inputs);
  auto intArgCover = intArgs(cover);
  Gecode::IntVarArgs countVars = intVarArgs(counts);

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
    if (shouldHold) {
      countVars << Gecode::IntVar(_space, 0, inputVars.size());
    }
  }
  unshare(_space, inputVars);
  if (shouldHold) {
    Gecode::count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
    return;
  }
  Gecode::IntVarArgs actualCounts(intArgCover.size());
  for (int i = 0; i < countVars.size(); i++) {
    actualCounts[i] =
        Gecode::IntVar(_space, 0, static_cast<int>(inputVars.size()));
  }
  Gecode::BoolVarArgs equalities(countVars.size());
  for (int i = 0; i < countVars.size(); i++) {
    equalities[i] = Gecode::BoolVar(_space, 0, 1);
    rel(_space, countVars[i], Gecode::IRT_EQ, actualCounts[i], equalities[i]);
  }
  rel(_space, Gecode::BOT_AND, equalities, 0);
  Gecode::count(_space, inputVars, actualCounts, intArgCover, Gecode::IPL_BND);
}

void GecodeSolver::fzn_global_cardinality_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_global_cardinality(inputs, cover, counts,
                                  boolVar(reified).val() == 1);
  }
  Gecode::IntVarArgs inputVars = intVarArgs(inputs);
  auto intArgCover = intArgs(cover);
  Gecode::IntVarArgs countVars = intVarArgs(counts);

  Gecode::Region re;
  const Gecode::IntSet coverSet(intArgCover);
  Gecode::IntSetRanges coverRegion(coverSet);
  auto* iv0_ri = re.alloc<Gecode::IntVarRanges>(inputVars.size());
  for (int i = inputVars.size(); i--;) {
    iv0_ri[i] = Gecode::IntVarRanges(inputVars[i]);
  }
  Gecode::Iter::Ranges::NaryUnion domainUnion(re, iv0_ri, inputVars.size());
  Gecode::Iter::Ranges::Diff<Gecode::Iter::Ranges::NaryUnion,
                             Gecode::IntSetRanges>
      extraRanges(domainUnion, coverRegion);
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::Diff<
      Gecode::Iter::Ranges::NaryUnion, Gecode::IntSetRanges>>
      extra(extraRanges);
  for (; extra(); ++extra) {
    intArgCover << extra.val();
  }
  unshare(_space, inputVars);
  Gecode::IntVarArgs actualCounts(intArgCover.size());
  for (int i = 0; i < countVars.size(); i++) {
    actualCounts[i] =
        Gecode::IntVar(_space, 0, static_cast<int>(inputs.size()));
  }
  Gecode::BoolVarArgs equalities(countVars.size());
  for (int i = 0; i < countVars.size(); i++) {
    equalities[i] = Gecode::BoolVar(_space, 0, 1);
    rel(_space, countVars[i], Gecode::IRT_EQ, actualCounts[i], equalities[i]);
  }
  rel(_space, Gecode::BOT_AND, equalities, boolVar(reified));
  Gecode::count(_space, inputVars, actualCounts, intArgCover, Gecode::IPL_BND);
}

void GecodeSolver::fzn_global_cardinality_closed(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const bool shouldHold) {
  if (shouldHold) {
    auto inputVars = intVarArgs(inputs);
    const auto intArgCover = intArgs(cover);
    const auto countVars = intVarArgs(counts);
    unshare(_space, inputVars);
    count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
    return;
  }
  Gecode::IntVarArgs inputVars = intVarArgs(inputs);
  auto intArgCover = intArgs(cover);
  Gecode::IntVarArgs countVars = intVarArgs(counts);

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
  }
  Gecode::IntVarArgs actualCounts(intArgCover.size());
  for (int i = 0; i < intArgCover.size(); i++) {
    actualCounts[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }

  unshare(_space, inputVars);
  Gecode::BoolVarArgs equalities(countVars.size() + inputVars.size());
  for (int i = 0; i < equalities.size(); i++) {
    equalities[i] = Gecode::BoolVar(_space, 0, 1);
    if (i < countVars.size()) {
      rel(_space, countVars[i], Gecode::IRT_EQ, actualCounts[i], equalities[i]);
    } else {
      rel(_space, actualCounts[i], Gecode::IRT_EQ, 0, equalities[i]);
    }
  }
  Gecode::count(_space, inputVars, actualCounts, intArgCover, Gecode::IPL_BND);
  rel(_space, Gecode::BOT_AND, equalities, 0);
}

void GecodeSolver::fzn_global_cardinality_closed_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_global_cardinality_closed(inputs, cover, counts,
                                         boolVar(reified).val() == 1);
  }
  auto inputVars = intVarArgs(inputs);
  auto intArgCover = intArgs(cover);
  auto countVars = intVarArgs(counts);

  Gecode::Region re;
  const Gecode::IntSet coverSet(intArgCover);
  Gecode::IntSetRanges coverRanges(coverSet);
  auto* inputDomains = re.alloc<Gecode::IntVarRanges>(inputVars.size());
  for (int i = inputVars.size(); i--;) {
    inputDomains[i] = Gecode::IntVarRanges(inputVars[i]);
  }
  Gecode::Iter::Ranges::NaryUnion domainUnion(re, inputDomains,
                                              inputVars.size());
  Gecode::Iter::Ranges::Diff<Gecode::Iter::Ranges::NaryUnion,
                             Gecode::IntSetRanges>
      extraRanges(domainUnion, coverRanges);
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::Diff<
      Gecode::Iter::Ranges::NaryUnion, Gecode::IntSetRanges>>
      extra(extraRanges);
  for (; extra(); ++extra) {
    intArgCover << extra.val();
  }
  Gecode::IntVarArgs actualCounts(intArgCover.size());
  for (int i = 0; i < intArgCover.size(); i++) {
    actualCounts[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }

  unshare(_space, inputVars);
  Gecode::BoolVarArgs equalities(countVars.size() + inputVars.size());
  for (int i = 0; i < equalities.size(); i++) {
    equalities[i] = Gecode::BoolVar(_space, 0, 1);
    if (i < countVars.size()) {
      rel(_space, countVars[i], Gecode::IRT_EQ, actualCounts[i], equalities[i]);
    } else {
      rel(_space, actualCounts[i], Gecode::IRT_EQ, 0, equalities[i]);
    }
  }
  Gecode::count(_space, inputVars, actualCounts, intArgCover, Gecode::IPL_BND);
  rel(_space, Gecode::BOT_AND, equalities, boolVar(reified));
}
void GecodeSolver::fzn_global_cardinality_low_up(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const bool shouldHold) {
  auto inputVars = intVarArgs(inputs);
  auto intArgCover = intArgs(cover);
  if (shouldHold) {
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
    return;
  }

  Gecode::Region re;
  const Gecode::IntSet coverSet(intArgCover);
  Gecode::IntSetRanges coverRanges(coverSet);
  auto* iv0_ri = re.alloc<Gecode::IntVarRanges>(inputVars.size());
  for (int i = inputVars.size(); i--;) {
    iv0_ri[i] = Gecode::IntVarRanges(inputVars[i]);
  }
  Gecode::Iter::Ranges::NaryUnion inputDomains(re, iv0_ri, inputVars.size());
  Gecode::Iter::Ranges::Diff<Gecode::Iter::Ranges::NaryUnion,
                             Gecode::IntSetRanges>
      domainUnion(inputDomains, coverRanges);
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::Diff<
      Gecode::Iter::Ranges::NaryUnion, Gecode::IntSetRanges>>
      extra(domainUnion);
  for (; extra(); ++extra) {
    intArgCover << extra.val();
  }
  Gecode::IntVarArgs extendedCounts(intArgCover.size());
  for (int i = 0; i < extendedCounts.size(); i++) {
    extendedCounts[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }
  unshare(_space, inputVars);
  Gecode::BoolVarArgs equalities(static_cast<int>(cover.size()));
  for (int i = 0; i < extendedCounts.size(); i++) {
    equalities[i] = Gecode::BoolVar(_space, 0, 1);
    dom(_space, extendedCounts[i], static_cast<int>(lowerBounds[i]),
        static_cast<int>(upperBounds[i]), equalities[i]);
  }
  rel(_space, Gecode::BOT_AND, equalities, 0);
  Gecode::count(_space, inputVars, extendedCounts, intArgCover,
                Gecode::IPL_BND);
}
void GecodeSolver::fzn_global_cardinality_low_up_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_global_cardinality_low_up(
        inputs, cover, lowerBounds, upperBounds, boolVar(reified).val() == 1);
  }
  auto inputVars = intVarArgs(inputs);
  auto intArgCover = intArgs(cover);

  Gecode::Region re;
  const Gecode::IntSet coverSet(intArgCover);
  Gecode::IntSetRanges cover_r(coverSet);
  auto* inputDomains = re.alloc<Gecode::IntVarRanges>(inputVars.size());
  for (int i = inputVars.size(); i--;) {
    inputDomains[i] = Gecode::IntVarRanges(inputVars[i]);
  }
  Gecode::Iter::Ranges::NaryUnion domainUnion(re, inputDomains,
                                              inputVars.size());
  Gecode::Iter::Ranges::Diff<Gecode::Iter::Ranges::NaryUnion,
                             Gecode::IntSetRanges>
      extraRanges(domainUnion, cover_r);
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::Diff<
      Gecode::Iter::Ranges::NaryUnion, Gecode::IntSetRanges>>
      extra(extraRanges);
  for (; extra(); ++extra) {
    intArgCover << extra.val();
  }
  Gecode::IntVarArgs countVars(intArgCover.size());
  for (int i = 0; i < countVars.size(); i++) {
    countVars[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }
  unshare(_space, inputVars);
  Gecode::BoolVarArgs equalities(static_cast<int>(cover.size()));
  for (int i = 0; i < countVars.size(); i++) {
    equalities[i] = Gecode::BoolVar(_space, 0, 1);
    dom(_space, countVars[i], static_cast<int>(lowerBounds[i]),
        static_cast<int>(upperBounds[i]), equalities[i]);
  }
  rel(_space, Gecode::BOT_AND, equalities, boolVar(reified));
  Gecode::count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
}

void GecodeSolver::fzn_global_cardinality_low_up_closed(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const bool shouldHold) {
  auto inputVars = intVarArgs(inputs);
  auto intArgCover = intArgs(cover);

  if (shouldHold) {
    const auto lbound = intArgs(lowerBounds);
    const auto ubound = intArgs(upperBounds);
    Gecode::IntSetArgs countBounds(intArgCover.size());
    for (int i = intArgCover.size(); i--;)
      countBounds[i] = Gecode::IntSet(lbound[i], ubound[i]);
    unshare(_space, inputVars);
    count(_space, inputVars, countBounds, intArgCover, Gecode::IPL_BND);
    return;
  }

  Gecode::Region re;
  const Gecode::IntSet coverSet(intArgCover);
  Gecode::IntSetRanges coverRanges(coverSet);
  auto* iv0_ri = re.alloc<Gecode::IntVarRanges>(inputVars.size());
  for (int i = inputVars.size(); i--;) {
    iv0_ri[i] = Gecode::IntVarRanges(inputVars[i]);
  }
  Gecode::Iter::Ranges::NaryUnion inputDomains(re, iv0_ri, inputVars.size());
  Gecode::Iter::Ranges::Diff<Gecode::Iter::Ranges::NaryUnion,
                             Gecode::IntSetRanges>
      domainUnion(inputDomains, coverRanges);
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::Diff<
      Gecode::Iter::Ranges::NaryUnion, Gecode::IntSetRanges>>
      extra(domainUnion);
  for (; extra(); ++extra) {
    intArgCover << extra.val();
  }
  Gecode::IntVarArgs countVars(intArgCover.size());
  for (int i = 0; i < countVars.size(); i++) {
    countVars[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }
  unshare(_space, inputVars);
  Gecode::BoolVarArgs equalities(countVars.size() + inputVars.size());
  for (int i = 0; i < equalities.size(); i++) {
    equalities[i] = Gecode::BoolVar(_space, 0, 1);
    if (i < countVars.size()) {
      dom(_space, countVars[i], static_cast<int>(lowerBounds[i]),
          static_cast<int>(upperBounds[i]), equalities[i]);
    } else {
      rel(_space, countVars[i], Gecode::IRT_EQ, 0, equalities[i]);
    }
  }
  rel(_space, Gecode::BOT_AND, equalities, 0);
  Gecode::count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
}

void GecodeSolver::fzn_global_cardinality_low_up_closed_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const ConstraintVarId reified) {
  if (boolVar(reified).assigned() && boolVar(reified).val()) {
    return fzn_global_cardinality_low_up_closed(
        inputs, cover, lowerBounds, upperBounds, boolVar(reified).val() == 1);
  }

  auto inputVars = intVarArgs(inputs);
  auto intArgCover = intArgs(cover);

  Gecode::Region re;
  const Gecode::IntSet coverSet(intArgCover);
  Gecode::IntSetRanges coverRanges(coverSet);
  auto* iv0_ri = re.alloc<Gecode::IntVarRanges>(inputVars.size());
  for (int i = inputVars.size(); i--;) {
    iv0_ri[i] = Gecode::IntVarRanges(inputVars[i]);
  }
  Gecode::Iter::Ranges::NaryUnion inputDomains(re, iv0_ri, inputVars.size());
  Gecode::Iter::Ranges::Diff<Gecode::Iter::Ranges::NaryUnion,
                             Gecode::IntSetRanges>
      domainUnion(inputDomains, coverRanges);
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::Diff<
      Gecode::Iter::Ranges::NaryUnion, Gecode::IntSetRanges>>
      extra(domainUnion);
  for (; extra(); ++extra) {
    intArgCover << extra.val();
  }
  Gecode::IntVarArgs countVars(intArgCover.size());
  for (int i = 0; i < countVars.size(); i++) {
    countVars[i] = Gecode::IntVar(_space, 0, inputVars.size());
  }
  unshare(_space, inputVars);
  Gecode::BoolVarArgs equalities(countVars.size() + inputVars.size());
  for (int i = 0; i < equalities.size(); i++) {
    equalities[i] = Gecode::BoolVar(_space, 0, 1);
    if (i < countVars.size()) {
      dom(_space, countVars[i], static_cast<int>(lowerBounds[i]),
          static_cast<int>(upperBounds[i]), equalities[i]);
    } else {
      rel(_space, countVars[i], Gecode::IRT_EQ, 0, equalities[i]);
    }
  }
  rel(_space, Gecode::BOT_AND, equalities, boolVar(reified));
  Gecode::count(_space, inputVars, countVars, intArgCover, Gecode::IPL_BND);
}

void GecodeSolver::fzn_count(const std::vector<ConstraintVarId>& inputs,
                             const Int needle, const RelationType relation,
                             const Int amount, const bool shouldHold) {
  Gecode::count(_space, intVarArgs(inputs), static_cast<int>(needle),
                toGecodeIntRelType(relationTypeConverse(relation), shouldHold),
                static_cast<int>(amount));
}

void GecodeSolver::fzn_count(const std::vector<ConstraintVarId>& inputs,
                             const Int needle, const RelationType relation,
                             const ConstraintVarId amount,
                             const bool shouldHold) {
  Gecode::count(_space, intVarArgs(inputs), static_cast<int>(needle),
                toGecodeIntRelType(relationTypeConverse(relation), shouldHold),
                intVar(amount));
}

void GecodeSolver::fzn_count(const std::vector<ConstraintVarId>& inputs,
                             const ConstraintVarId needle,
                             const RelationType relation, const Int amount,
                             const bool shouldHold) {
  Gecode::count(_space, intVarArgs(inputs), intVar(needle),
                toGecodeIntRelType(relationTypeConverse(relation), shouldHold),
                static_cast<int>(amount));
}

void GecodeSolver::fzn_count(const std::vector<ConstraintVarId>& inputs,
                             const ConstraintVarId needle,
                             const RelationType relation,
                             const ConstraintVarId amount,
                             const bool shouldHold) {
  Gecode::count(_space, intVarArgs(inputs), intVar(needle),
                toGecodeIntRelType(relationTypeConverse(relation), shouldHold),
                intVar(amount));
}

void GecodeSolver::fzn_count_reif(const std::vector<ConstraintVarId>& inputs,
                                  const Int needle, const RelationType relation,
                                  const Int amount,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_count(inputs, needle, relation, amount,
                     boolVar(reified).val() == 1);
  }
  const Gecode::IntVar c(_space, 0, Gecode::Int::Limits::max);
  count(_space, intVarArgs(inputs), static_cast<int>(needle), Gecode::IRT_EQ,
        c);
  rel(_space, c, toGecodeIntRelType(relationTypeConverse(relation)),
      static_cast<int>(amount), boolVar(reified));
}

void GecodeSolver::fzn_count_reif(const std::vector<ConstraintVarId>& inputs,
                                  const Int needle, const RelationType relation,
                                  const ConstraintVarId amount,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_count(inputs, needle, relation, amount,
                     boolVar(reified).val() == 1);
  }
  const Gecode::IntVar c(_space, 0, Gecode::Int::Limits::max);
  count(_space, intVarArgs(inputs), static_cast<int>(needle), Gecode::IRT_EQ,
        c);
  rel(_space, c, toGecodeIntRelType(relationTypeConverse(relation)),
      intVar(amount), boolVar(reified));
}

void GecodeSolver::fzn_count_reif(const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId needle,
                                  const RelationType relation, const Int amount,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_count(inputs, needle, relation, amount,
                     boolVar(reified).val() == 1);
  }
  const Gecode::IntVar c(_space, 0, Gecode::Int::Limits::max);
  count(_space, intVarArgs(inputs), intVar(needle), Gecode::IRT_EQ, c);
  rel(_space, c, toGecodeIntRelType(relationTypeConverse(relation)),
      static_cast<int>(amount), boolVar(reified));
}

void GecodeSolver::fzn_count_reif(const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId needle,
                                  RelationType relation,
                                  const ConstraintVarId amount,
                                  const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_count(inputs, needle, relation, amount,
                     boolVar(reified).val() == 1);
  }
  const Gecode::IntVar c(_space, 0, Gecode::Int::Limits::max);
  count(_space, intVarArgs(inputs), intVar(needle), Gecode::IRT_EQ, c);
  rel(_space, c, toGecodeIntRelType(relationTypeConverse(relation)),
      intVar(amount), boolVar(reified));
}

void GecodeSolver::fzn_table_bool(const std::vector<ConstraintVarId>& inputs,
                                  const std::vector<std::vector<bool>>& table,
                                  const bool shouldHold) {
  auto inputVars = intVarArgs(inputs);
  const Gecode::TupleSet ts = tupleSet(table);
  Gecode::unshare(_space, inputVars);
  extensional(_space, inputVars, ts, shouldHold);
}

void GecodeSolver::fzn_table_bool_reif(
    const std::vector<ConstraintVarId>& inputs,
    const std::vector<std::vector<Int>>& table, const ConstraintVarId reified) {
  auto inputVars = intVarArgs(inputs);
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
  auto inputVars = intVarArgs(inputs);
  const Gecode::TupleSet ts = tupleSet(table);
  Gecode::unshare(_space, inputVars);
  extensional(_space, inputVars, ts,
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV));
}

void GecodeSolver::set_in(const ConstraintVarId varId,
                          const SortedUniqueVector& values,
                          const bool shouldHold) {
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
