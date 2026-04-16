#include "atlantis/invariantgraph/gecodeSolver.hpp"

#include <ranges>
#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/utils/domains.hpp"
#include "atlantis/utils/gecode_compat.hpp"

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

Gecode::BoolVarArgs GecodeSolver::boolVarArgs(
    const std::vector<ConstraintVarId>& varIds) {
  Gecode::BoolVarArgs args(static_cast<int>(varIds.size()));
  for (int i = 0; i < static_cast<int>(varIds.size()); ++i) {
    assert(varIds.at(i).isBoolVar());
    args[i] = boolVar(size_t{varIds[i]});
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

void GecodeSolver::array_bool_rel(const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId reified,
                                  const Gecode::BoolOpType op) {
  rel(_space, op, boolVarArgs(inputs), boolVar(reified), Gecode::IPL_BND);
}

void GecodeSolver::array_bool_rel(const std::vector<ConstraintVarId>& inputs,
                                  const bool shouldHold,
                                  const Gecode::BoolOpType op) {
  rel(_space, op, boolVarArgs(inputs), shouldHold ? 1 : 0, Gecode::IPL_BND);
}

void GecodeSolver::array_bool_and(const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId reified) {
  array_bool_rel(inputs, reified, Gecode::BOT_AND);
}

void GecodeSolver::array_bool_and(const std::vector<ConstraintVarId>& inputs,
                                  const bool shouldHold) {
  array_bool_rel(inputs, shouldHold, Gecode::BOT_AND);
}

void GecodeSolver::array_bool_element(const ConstraintVarId& index,
                                      const std::vector<Int>& parameters,
                                      const ConstraintVarId output,
                                      const Int offset) {
  Gecode::IntSharedArray sia(static_cast<int>(parameters.size()));
  for (int i = 0; i < static_cast<int>(parameters.size()); ++i) {
    sia[i] = static_cast<int>(parameters[i]);
  }
  element(_space, sia, intVar(index), -static_cast<int>(offset),
          boolVar(output));
}

void GecodeSolver::array_bool_element2d(
    const ConstraintVarId& rowIndex, const ConstraintVarId& colIndex,
    const std::vector<std::vector<Int>>& parameters,
    const ConstraintVarId output, const Int rowOffset, const Int colOffset) {
  Gecode::IntSharedArray sia(
      static_cast<int>(parameters.size() * parameters.front().size()));
  int i = 0;
  for (const auto& row : parameters) {
    for (const Int val : row) {
      sia[i++] = static_cast<int>(val);
    }
  }
  element(_space, sia, intVar(colIndex), -static_cast<int>(colOffset),
          static_cast<int>(parameters.front().size()), intVar(rowIndex),
          -static_cast<int>(rowOffset), static_cast<int>(parameters.size()),
          boolVar(output), Gecode::IPL_DOM);
}

void GecodeSolver::array_bool_or(const std::vector<ConstraintVarId>& inputs,
                                 const ConstraintVarId reified) {
  array_bool_rel(inputs, reified, Gecode::BOT_OR);
}

void GecodeSolver::array_bool_or(const std::vector<ConstraintVarId>& inputs,
                                 const bool shouldHold) {
  array_bool_rel(inputs, shouldHold, Gecode::BOT_OR);
}

void GecodeSolver::array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                                  const ConstraintVarId reified) {
  array_bool_rel(inputs, reified, Gecode::BOT_XOR);
}

void GecodeSolver::array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                                  const bool shouldHold) {
  array_bool_rel(inputs, shouldHold, Gecode::BOT_XOR);
}

void GecodeSolver::bool2int(const ConstraintVarId boolVarId,
                            const ConstraintVarId intVarId) {
  channel(_space, boolVar(boolVarId), intVar(intVarId));
}

}  // namespace atlantis::invariantgraph
