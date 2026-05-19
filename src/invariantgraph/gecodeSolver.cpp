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

GecodeSolver::GecodeSolver() = default;

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

size_t GecodeSolver::numIntVars() const { return _space._iv.size(); }

size_t GecodeSolver::numBoolVars() const { return _space._bv.size(); }

Gecode::IntVar& GecodeSolver::intVar(const size_t varId) {
  assert(varId < _space._iv.size());
  return _space._iv[varId];
}

Gecode::IntVar& GecodeSolver::intVar(const ConstraintVarId varId) {
  assert(varId.isIntVar());
  return intVar(size_t{varId});
}

Gecode::BoolVar& GecodeSolver::boolVar(const size_t varId) {
  assert(varId < _space._bv.size());
  return _space._bv[size_t{varId}];
}

Gecode::BoolVar& GecodeSolver::boolVar(const ConstraintVarId varId) {
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

Gecode::IntSharedArray GecodeSolver::intSharedArray(
    const std::vector<bool>& valVector) {
  Gecode::IntSharedArray pars(static_cast<int>(valVector.size()));
  for (int i = 0; i < static_cast<int>(valVector.size()); ++i) {
    pars[i] = valVector[i] ? 1 : 0;
  }
  return pars;
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

void GecodeSolver::fixPoint() {
  Gecode::SpaceStatus s = _space.status();
  if (s == Gecode::SS_FAILED) {
    throw InconsistencyException("UNSAT");
  }
  assert(s == Gecode::SS_SOLVED);
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

void GecodeSolver::array_bool_op(const std::vector<ConstraintVarId>& inputs,
                                 const ConstraintVarId reified,
                                 const Gecode::BoolOpType op) {
  rel(_space, op, boolVarArgs(inputs), boolVar(reified), Gecode::IPL_BND);
}

void GecodeSolver::array_bool_op(const std::vector<ConstraintVarId>& inputs,
                                 const bool shouldHold,
                                 const Gecode::BoolOpType op) {
  rel(_space, op, boolVarArgs(inputs), shouldHold ? 1 : 0, Gecode::IPL_BND);
}

void GecodeSolver::bool_op(const ConstraintVarId b1, const ConstraintVarId b2,
                           const ConstraintVarId reified,
                           const Gecode::BoolOpType op) {
  rel(_space, boolVar(b1), op, boolVar(b2), boolVar(reified), Gecode::IPL_BND);
}

void GecodeSolver::bool_op(const ConstraintVarId b1, const ConstraintVarId b2,
                           const bool shouldHold, const Gecode::BoolOpType op) {
  rel(_space, boolVar(b1), op, boolVar(b2), shouldHold, Gecode::IPL_BND);
}

void GecodeSolver::bool_rel(const ConstraintVarId b1, const ConstraintVarId b2,
                            const ConstraintVarId reified,
                            const Gecode::IntRelType irt) {
  rel(_space, boolVar(b1), irt, boolVar(b2),
      Gecode::Reify(boolVar(reified), Gecode::RM_EQV), Gecode::IPL_BND);
}

void GecodeSolver::bool_rel(const ConstraintVarId b1, const ConstraintVarId b2,
                            const bool shouldHold,
                            const Gecode::IntRelType irt) {
  rel(_space, boolVar(b1), shouldHold ? irt : neg(irt), boolVar(b2),
      Gecode::IPL_BND);
}

void GecodeSolver::bool_lin_rel(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                const Int rhs, const bool shouldHold,
                                const Gecode::IntRelType irt) {
  linear(_space, intSharedArray(coeffs), boolVarArgs(inputs),
         shouldHold ? irt : neg(irt), static_cast<int>(rhs), Gecode::IPL_BND);
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
  element(_space, intSharedArray(parameters), intVar(index),
          -static_cast<int>(offset), boolVar(output), Gecode::IPL_DOM);
}

void GecodeSolver::array_bool_element2d(
    const ConstraintVarId rowIndex, const ConstraintVarId colIndex,
    const std::vector<std::vector<bool>>& parameters,
    const ConstraintVarId output, const Int rowOffset, const Int colOffset) {
  element(_space, intSharedArray(parameters), intVar(colIndex),
          -static_cast<int>(colOffset),
          static_cast<int>(parameters.front().size()), intVar(rowIndex),
          -static_cast<int>(rowOffset), static_cast<int>(parameters.size()),
          boolVar(output), Gecode::IPL_DOM);
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
  element(_space, intSharedArray(parameters), intVar(index),
          -static_cast<int>(offset), intVar(output));
}

void GecodeSolver::array_int_element2d(
    const ConstraintVarId rowIndex, const ConstraintVarId colIndex,
    const std::vector<std::vector<Int>>& parameters,
    const ConstraintVarId output, const Int rowOffset, const Int colOffset) {
  element(_space, intSharedArray(parameters), intVar(colIndex),
          -static_cast<int>(colOffset),
          static_cast<int>(parameters.front().size()), intVar(rowIndex),
          -static_cast<int>(rowOffset), static_cast<int>(parameters.size()),
          intVar(output), Gecode::IPL_DOM);
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
  element(_space, boolVarArgs(inputs), intVar(index), -static_cast<int>(offset),
          boolVar(output), Gecode::IPL_DOM);
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
  element(_space, boolVarArgs(inputs), intVar(colIndex),
          -static_cast<int>(colOffset), static_cast<int>(inputs.front().size()),
          intVar(rowIndex), -static_cast<int>(rowOffset),
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
  element(_space, intVarArgs(inputs), intVar(index), -static_cast<int>(offset),
          intVar(output));
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
  element(_space, intVarArgs(inputs), intVar(colIndex),
          -static_cast<int>(colOffset), static_cast<int>(inputs.front().size()),
          intVar(rowIndex), -static_cast<int>(rowOffset),
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
                               const Int rhs, bool shouldHold) {
  bool_lin_rel(coeffs, inputs, rhs, shouldHold, Gecode::IRT_EQ);
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

}  // namespace atlantis::invariantgraph
