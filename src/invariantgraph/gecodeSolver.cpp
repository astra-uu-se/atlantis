#include "atlantis/invariantgraph/gecodeSolver.hpp"

#include <gecode/minimodel.hh>
#include <numeric>
#include <ranges>
#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/utils/domains.hpp"

using atlantis::propagation::SolverBase;

namespace atlantis::invariantgraph {

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

void GecodeSolver::bool_rel(const ConstraintVarId lhs,
                            const ConstraintVarId rhs,
                            const ConstraintVarId reified,
                            const Gecode::IntRelType irt) {
  Gecode::rel(_space, boolVar(lhs), irt, boolVar(rhs),
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::bool_rel(const ConstraintVarId lhs,
                            const ConstraintVarId rhs, const bool shouldHold,
                            const Gecode::IntRelType irt) {
  Gecode::rel(_space, boolVar(lhs), shouldHold ? irt : neg(irt), boolVar(rhs),
              Gecode::IPL_BND);
}

void GecodeSolver::int_rel(const ConstraintVarId lhs, const ConstraintVarId rhs,
                           const ConstraintVarId reified,
                           const Gecode::IntRelType irt) {
  Gecode::rel(_space, intVar(lhs), irt, intVar(rhs),
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::int_rel(const ConstraintVarId lhs, const Int rhs,
                           const ConstraintVarId reified,
                           const Gecode::IntRelType irt) {
  Gecode::rel(_space, intVar(lhs), irt, static_cast<int>(rhs),
              Gecode::Reify(boolVar(reified), Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::int_rel(const ConstraintVarId lhs, const ConstraintVarId rhs,
                           const bool shouldHold,
                           const Gecode::IntRelType irt) {
  Gecode::rel(_space, intVar(lhs), shouldHold ? irt : neg(irt), intVar(rhs),
              Gecode::IPL_BND);
}

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const Int rhs, const ConstraintVarId reified,
                                const Gecode::IntRelType irt) {
  const auto& reif = boolVar(reified);
  if (reif.assigned()) {
    return bool_lin_rel(coeffs, inputs, rhs, reif.val() == 1, irt);
  }
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs), irt,
         static_cast<int>(rhs), Gecode::Reify(reif, Gecode::RM_EQV),
         Gecode::IPL_BND);
}

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const ConstraintVarId rhs, const Int rhsOffset,
                                const Gecode::IntRelType irt) {
  const auto rhsVar = expr(_space, intVar(rhs) + static_cast<int>(rhsOffset));
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs), irt, rhsVar);
}

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const Int rhs, const bool shouldHold,
                                const Gecode::IntRelType irt) {
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
         shouldHold ? irt : neg(irt), static_cast<int>(rhs));
}

void GecodeSolver::int_lin_rel(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const Int rhs, const ConstraintVarId reified,
                               const Gecode::IntRelType irt) {
  const auto& reif = boolVar(reified);
  if (reif.assigned()) {
    return int_lin_rel(coeffs, inputs, rhs, reif.val() == 1, irt);
  }
  linear(_space, intSharedArray(coeffs), intVarArgs(inputs), irt,
         static_cast<int>(rhs), Gecode::Reify(reif, Gecode::RM_EQV),
         Gecode::IPL_BND);
}

void GecodeSolver::int_lin_rel(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const ConstraintVarId rhs, const Int rhsOffset,
                               const Gecode::IntRelType irt) {
  const auto rhsVar = expr(_space, intVar(rhs) + static_cast<int>(rhsOffset));
  linear(_space, intSharedArray(coeffs), intVarArgs(inputs), irt, rhsVar);
}

void GecodeSolver::int_lin_rel(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const Int rhs, const bool shouldHold,
                               const Gecode::IntRelType irt) {
  linear(_space, intSharedArray(coeffs), intVarArgs(inputs),
         shouldHold ? irt : neg(irt), static_cast<int>(rhs));
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
  for (int v = _space._iv[size_t{varId}].min();
       v <= _space._iv[size_t{varId}].max(); ++v) {
    if (_space._iv[size_t{varId}].in(v)) {
      values[i++] = v;
    }
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
  Gecode::SpaceStatus s = _space.status();
  if (s == Gecode::SS_FAILED) {
    throw InconsistencyException("UNSAT");
  }
  assert(s == Gecode::SS_SOLVED);
}

void GecodeSolver::array_bool_and(const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId reified) {
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
  const auto rowIndexVar =
      Gecode::expr(_space, intVar(rowIndex) - static_cast<int>(rowOffset));
  const auto colIndexVar =
      Gecode::expr(_space, intVar(colIndex) - static_cast<int>(colOffset));
  Gecode::element(_space, intSharedArray(parameters), colIndexVar,
                  static_cast<int>(parameters.front().size()), rowIndexVar,
                  static_cast<int>(parameters.size()), boolVar(output),
                  Gecode::IPL_DOM);
}

void GecodeSolver::array_bool_or(const std::vector<ConstraintVarId>& inputs,
                                 const ConstraintVarId reified) {
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
      [&](const ConstraintVarId varId) { return boolVar(varId).one(); });
  if (allParams) {
    std::vector<bool> params(inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
      params[i] = boolVar(inputs[i]).val() == 1;
    }
    array_bool_element(index, params, output, offset);
    return;
  }
  const auto indexVar =
      Gecode::expr(_space, intVar(index) - static_cast<int>(offset));
  Gecode::element(_space, boolVarArgs(inputs), indexVar, boolVar(output),
                  Gecode::IPL_DOM);
}

void GecodeSolver::array_var_bool_element2d(
    const ConstraintVarId rowIndex, const ConstraintVarId colIndex,
    const std::vector<std::vector<ConstraintVarId>>& inputs,
    const ConstraintVarId output, const Int rowOffset, const Int colOffset) {
  const bool allParams =
      std::ranges::all_of(inputs, [&](const std::vector<ConstraintVarId>& row) {
        return std::ranges::all_of(row, [&](const ConstraintVarId varId) {
          return boolVar(varId).one();
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
  const auto rowIndexVar =
      Gecode::expr(_space, intVar(rowIndex) - static_cast<int>(rowOffset));
  const auto colIndexVar =
      Gecode::expr(_space, intVar(colIndex) - static_cast<int>(colOffset));
  Gecode::element(_space, boolVarArgs(inputs), colIndexVar,
                  static_cast<int>(inputs.front().size()), rowIndexVar,
                  static_cast<int>(inputs.size()), boolVar(output));
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
  bool_rel(b1, b2, shouldHold, Gecode::IRT_EQ);
}

void GecodeSolver::bool_eq_reif(const ConstraintVarId b1,
                                const ConstraintVarId b2,
                                const ConstraintVarId reified) {
  bool_rel(b1, b2, reified, Gecode::IRT_EQ);
}

void GecodeSolver::bool_le(const ConstraintVarId b1, const ConstraintVarId b2,
                           const bool shouldHold) {
  bool_rel(b1, b2, shouldHold, Gecode::IRT_LQ);
}

void GecodeSolver::bool_le_reif(const ConstraintVarId b1,
                                const ConstraintVarId b2,
                                const ConstraintVarId reified) {
  bool_rel(b1, b2, reified, Gecode::IRT_LQ);
}

void GecodeSolver::bool_lin_eq(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const Int rhs, const bool shouldHold) {
  bool_lin_rel(coeffs, inputs, rhs, shouldHold, Gecode::IRT_EQ);
}

void GecodeSolver::bool_lin_eq(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const ConstraintVarId rhs, const Int rhsOffset) {
  bool_lin_rel(coeffs, inputs, rhs, rhsOffset, Gecode::IRT_EQ);
}

void GecodeSolver::bool_lin_eq_reif(const std::vector<Int>& coeffs,
                                    const std::vector<ConstraintVarId>& inputs,
                                    const Int rhs,
                                    const ConstraintVarId reified) {
  bool_lin_rel(coeffs, inputs, rhs, reified, Gecode::IRT_EQ);
}

void GecodeSolver::bool_lin_le(const std::vector<Int>& coeffs,
                               const std::vector<ConstraintVarId>& inputs,
                               const Int rhs, const bool shouldHold) {
  bool_lin_rel(coeffs, inputs, rhs, shouldHold, Gecode::IRT_LQ);
}

void GecodeSolver::bool_lin_le_reif(const std::vector<Int>& coeffs,
                                    const std::vector<ConstraintVarId>& inputs,
                                    const Int rhs,
                                    const ConstraintVarId reified) {
  bool_lin_rel(coeffs, inputs, rhs, reified, Gecode::IRT_LQ);
}

void GecodeSolver::bool_lt(const ConstraintVarId b1, const ConstraintVarId b2,
                           const bool shouldHold) {
  bool_rel(b1, b2, shouldHold, Gecode::IRT_LE);
}

void GecodeSolver::bool_lt_reif(const ConstraintVarId b1,
                                const ConstraintVarId b2,
                                const ConstraintVarId reified) {
  bool_rel(b1, b2, reified, Gecode::IRT_LE);
}

void GecodeSolver::bool_not(const ConstraintVarId b1, const ConstraintVarId b2,
                            const bool shouldHold) {
  bool_rel(b1, b2, shouldHold, Gecode::IRT_NQ);
}

void GecodeSolver::bool_not_reif(const ConstraintVarId b1,
                                 const ConstraintVarId b2,
                                 const ConstraintVarId reified) {
  bool_rel(b1, b2, reified, Gecode::IRT_NQ);
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
  div(_space, arr[0], arr[1], arr[2]);
}

void GecodeSolver::int_eq(const ConstraintVarId lhs, const ConstraintVarId rhs,
                          const bool shouldHold) {
  int_rel(lhs, rhs, shouldHold, Gecode::IRT_EQ);
}

void GecodeSolver::int_eq_reif(const ConstraintVarId lhs,
                               const ConstraintVarId rhs,
                               const ConstraintVarId reified) {
  int_rel(lhs, rhs, reified, Gecode::IRT_EQ);
}

void GecodeSolver::int_eq_reif(const ConstraintVarId lhs, const Int rhs,
                               const ConstraintVarId reified) {
  int_rel(lhs, rhs, reified, Gecode::IRT_EQ);
}

void GecodeSolver::int_le(const ConstraintVarId lhs, const ConstraintVarId rhs,
                          const bool shouldHold) {
  int_rel(lhs, rhs, shouldHold, Gecode::IRT_LQ);
}

void GecodeSolver::int_le_reif(const ConstraintVarId lhs,
                               const ConstraintVarId rhs,
                               const ConstraintVarId reified) {
  int_rel(lhs, rhs, reified, Gecode::IRT_LQ);
}

void GecodeSolver::int_lin_eq(const std::vector<Int>& coeffs,
                              const std::vector<ConstraintVarId>& inputs,
                              const Int rhs, const bool shouldHold) {
  int_lin_rel(coeffs, inputs, rhs, shouldHold, Gecode::IRT_EQ);
}

void GecodeSolver::int_lin_eq(const std::vector<Int>& coeffs,
                              const std::vector<ConstraintVarId>& inputs,
                              const ConstraintVarId rhs, const Int rhsOffset) {
  int_lin_rel(coeffs, inputs, rhs, rhsOffset, Gecode::IRT_EQ);
}

void GecodeSolver::int_lin_eq_reif(const std::vector<Int>& coeffs,
                                   const std::vector<ConstraintVarId>& inputs,
                                   const Int rhs,
                                   const ConstraintVarId reified) {
  int_lin_rel(coeffs, inputs, rhs, reified, Gecode::IRT_EQ);
}

void GecodeSolver::int_lin_le(const std::vector<Int>& coeffs,
                              const std::vector<ConstraintVarId>& inputs,
                              const Int rhs, const bool shouldHold) {
  int_lin_rel(coeffs, inputs, rhs, shouldHold, Gecode::IRT_LQ);
}

void GecodeSolver::int_lin_le_reif(const std::vector<Int>& coeffs,
                                   const std::vector<ConstraintVarId>& inputs,
                                   const Int rhs,
                                   const ConstraintVarId reified) {
  int_lin_rel(coeffs, inputs, rhs, reified, Gecode::IRT_LQ);
}

void GecodeSolver::int_lin_ne(const std::vector<Int>& coeffs,
                              const std::vector<ConstraintVarId>& inputs,
                              const Int rhs, const bool shouldHold) {
  int_lin_rel(coeffs, inputs, rhs, shouldHold, Gecode::IRT_NQ);
}
void GecodeSolver::int_lin_ne_reif(const std::vector<Int>& coeffs,
                                   const std::vector<ConstraintVarId>& inputs,
                                   const Int rhs,
                                   const ConstraintVarId reified) {
  int_lin_rel(coeffs, inputs, rhs, reified, Gecode::IRT_NQ);
}

void GecodeSolver::int_lt(const ConstraintVarId lhs, const ConstraintVarId rhs,
                          const bool shouldHold) {
  int_rel(lhs, rhs, shouldHold, Gecode::IRT_LE);
}

void GecodeSolver::int_lt_reif(const ConstraintVarId lhs,
                               const ConstraintVarId rhs,
                               const ConstraintVarId reified) {
  int_rel(lhs, rhs, reified, Gecode::IRT_LE);
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
  int_rel(lhs, rhs, shouldHold, Gecode::IRT_NQ);
}

void GecodeSolver::int_ne_reif(const ConstraintVarId lhs,
                               const ConstraintVarId rhs,
                               const ConstraintVarId reified) {
  int_rel(lhs, rhs, reified, Gecode::IRT_NQ);
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
  if (!shouldHold) {
    return;
  }
  Gecode::IntVarArgs iv0 = intVarArgs(inputs);
  auto gecodeCover = intArgs(cover);
  Gecode::IntVarArgs iv1 = intVarArgs(counts);

  Gecode::Region re;
  const Gecode::IntSet cover_s(gecodeCover);
  Gecode::IntSetRanges cover_r(cover_s);
  auto* iv0_ri = re.alloc<Gecode::IntVarRanges>(iv0.size());
  for (int i=iv0.size(); i--;) {
    iv0_ri[i] = Gecode::IntVarRanges(iv0[i]);
  }
  Gecode::Iter::Ranges::NaryUnion iv0_r(re,iv0_ri,iv0.size());
  Gecode::Iter::Ranges::Diff<Gecode::Iter::Ranges::NaryUnion,Gecode::IntSetRanges>
    extra_r(iv0_r,cover_r);
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::Diff<
    Gecode::Iter::Ranges::NaryUnion,Gecode::IntSetRanges> > extra(extra_r);
  for (; extra(); ++extra) {
    gecodeCover << extra.val();
    iv1 << Gecode::IntVar(_space,0,iv0.size());
  }
  unshare(_space, iv0);
  Gecode::count(_space, iv0, iv1, gecodeCover, Gecode::IPL_BND);
}

void GecodeSolver::fzn_global_cardinality_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_global_cardinality(inputs, cover, counts, boolVar(reified).val() == 1);
  }
}

void GecodeSolver::fzn_global_cardinality_closed(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const bool shouldHold) {
  if (!shouldHold) {
    return;
  }
  auto iv0 = intVarArgs(inputs);
  const auto gecodeCover = intSharedArray(cover);
  const auto iv1 = intVarArgs(counts);
  unshare(_space, iv0);
  count(_space, iv0, iv1, gecodeCover, Gecode::IPL_BND);
}

void GecodeSolver::fzn_global_cardinality_closed_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<ConstraintVarId>& counts, const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_global_cardinality_closed(inputs, cover, counts, boolVar(reified).val() == 1);
  }
}
void GecodeSolver::fzn_global_cardinality_low_up(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& ccover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const bool shouldHold) {
  if (!shouldHold) {
    return;
  }
  auto x = intVarArgs(inputs);
  auto gecodeCover = intArgs(ccover);

  const auto lbound = intSharedArray(lowerBounds);
  const auto ubound = intSharedArray(upperBounds);
  Gecode::IntSetArgs y(gecodeCover.size());
  for (int i=gecodeCover.size(); i--;) {
    y[i] = Gecode::IntSet(lbound[i],ubound[i]);
  }

  const Gecode::IntSet cover_s(gecodeCover);
  Gecode::Region re;
  auto* xrs = re.alloc<Gecode::IntVarRanges>(x.size());
  for (int i=x.size(); i--;) {
    xrs[i].init(x[i]);
  }
  Gecode::Iter::Ranges::NaryUnion u(re, xrs, x.size());
  Gecode::Iter::Ranges::ToValues<Gecode::Iter::Ranges::NaryUnion> uv(u);
  for (; uv(); ++uv) {
    if (!cover_s.in(uv.val())) {
      gecodeCover << uv.val();
      y << Gecode::IntSet(0,x.size());
    }
  }
  unshare(_space, x);
  count(_space, x, y, gecodeCover, Gecode::IPL_BND);
}
void GecodeSolver::fzn_global_cardinality_low_up_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const ConstraintVarId reified) {
  if (boolVar(reified).assigned()) {
    return fzn_global_cardinality_low_up(inputs, cover, lowerBounds, upperBounds, boolVar(reified).val() == 1);
  }
}

void GecodeSolver::fzn_global_cardinality_low_up_closed(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const bool shouldHold) {
  if (!shouldHold) {
    return;
  }
  auto x = intVarArgs(inputs);
  const auto gecodeCover = intArgs(cover);

  const auto lbound = intArgs(lowerBounds);
  const auto ubound = intArgs(upperBounds);
  Gecode::IntSetArgs y(gecodeCover.size());
  for (int i = gecodeCover.size(); i--;) y[i] = Gecode::IntSet(lbound[i], ubound[i]);
  unshare(_space, x);
  count(_space, x, y, gecodeCover, Gecode::IPL_BND);
}

void GecodeSolver::fzn_global_cardinality_low_up_closed_reif(
    const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
    const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
    const ConstraintVarId reified) {
  if (boolVar(reified).assigned() && boolVar(reified).val()) {
    return fzn_global_cardinality_low_up_closed(inputs, cover, lowerBounds, upperBounds, boolVar(reified).val() == 1);
  }
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
  return Gecode::nvalues(_space, intVarArgs(inputs), Gecode::IRT_LE, static_cast<int>(numVals));
}

}  // namespace atlantis::invariantgraph
