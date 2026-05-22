#pragma once

#include <vector>

#include "varNode.hpp"

namespace atlantis::invariantgraph {

class ConstraintSolver {
 public:
  virtual ~ConstraintSolver() = default;

  [[nodiscard]] virtual size_t numIntVars() const = 0;
  [[nodiscard]] virtual size_t numBoolVars() const = 0;

  virtual ConstraintVarId newIntVar(Int) = 0;
  virtual ConstraintVarId newIntVar(const SearchDomain&) = 0;

  virtual ConstraintVarId newBoolVar(bool) = 0;
  virtual ConstraintVarId newBoolVar() = 0;

  virtual void fixPoint() = 0;

  [[nodiscard]] virtual SearchDomain intVarDomain(ConstraintVarId) const = 0;
  [[nodiscard]] virtual SearchDomain boolVarDomain(ConstraintVarId) const = 0;
  [[nodiscard]] virtual SearchDomain varDomain(ConstraintVarId) const = 0;

  // Constraints
  virtual void array_bool_and(const std::vector<ConstraintVarId>& inputs,
                              ConstraintVarId reified) = 0;
  virtual void array_bool_and(const std::vector<ConstraintVarId>& inputs,
                              bool shouldHold) = 0;
  virtual void array_bool_element(ConstraintVarId index,
                                  const std::vector<bool>& parameters,
                                  ConstraintVarId output, Int offset) = 0;
  virtual void array_bool_element2d(
      ConstraintVarId index1, ConstraintVarId index2,
      const std::vector<std::vector<bool>>& parameters, ConstraintVarId output,
      Int rowOffset, Int colOffset) = 0;
  virtual void array_bool_or(const std::vector<ConstraintVarId>& inputs,
                             ConstraintVarId reified) = 0;
  virtual void array_bool_or(const std::vector<ConstraintVarId>& inputs,
                             bool shouldHold) = 0;
  virtual void array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                              ConstraintVarId reified) = 0;
  virtual void array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                              bool shouldHold) = 0;
  virtual void bool2int(ConstraintVarId boolVarId,
                        ConstraintVarId intVarId) = 0;
  virtual void array_int_element(ConstraintVarId index,
                                 const std::vector<Int>& parameters,
                                 ConstraintVarId output, Int offset) = 0;
  virtual void array_int_element2d(
      ConstraintVarId index1, ConstraintVarId index2,
      const std::vector<std::vector<Int>>& parameters, ConstraintVarId output,
      Int rowOffset, Int colOffset) = 0;
  virtual void array_int_maximum(const std::vector<ConstraintVarId>& inputs,
                                 ConstraintVarId output) = 0;
  virtual void array_int_minimum(const std::vector<ConstraintVarId>& inputs,
                                 ConstraintVarId output) = 0;
  virtual void array_var_bool_element(
      ConstraintVarId index, const std::vector<ConstraintVarId>& inputs,
      ConstraintVarId output, Int offset) = 0;
  virtual void array_var_bool_element2d(
      ConstraintVarId rowIndex, ConstraintVarId colIndex,
      const std::vector<std::vector<ConstraintVarId>>& inputs,
      ConstraintVarId output, Int rowOffset, Int colOffset) = 0;
  virtual void array_var_int_element(ConstraintVarId index,
                                     const std::vector<ConstraintVarId>& inputs,
                                     ConstraintVarId output, Int offset) = 0;
  virtual void array_var_int_element2d(
      ConstraintVarId rowIndex, ConstraintVarId colIndex,
      const std::vector<std::vector<ConstraintVarId>>& inputs,
      ConstraintVarId output, Int rowOffset, Int colOffset) = 0;

  virtual void bool_and(ConstraintVarId b1, ConstraintVarId b2,
                        bool shouldHold) = 0;

  virtual void bool_and_reif(ConstraintVarId b1, ConstraintVarId b2,
                             ConstraintVarId reified) = 0;

  virtual void bool_clause(const std::vector<ConstraintVarId>& posInputs,
                           const std::vector<ConstraintVarId>& negInputs,
                           bool shouldHold) = 0;

  virtual void bool_clause_reif(const std::vector<ConstraintVarId>& posInputs,
                                const std::vector<ConstraintVarId>& negInputs,
                                ConstraintVarId reif) = 0;

  virtual void bool_eq(ConstraintVarId b1, ConstraintVarId b2,
                       bool shouldHold) = 0;

  virtual void bool_eq_reif(ConstraintVarId b1, ConstraintVarId b2,
                            ConstraintVarId reif) = 0;

  virtual void bool_le(ConstraintVarId b1, ConstraintVarId b2,
                       bool shouldHold) = 0;

  virtual void bool_le_reif(ConstraintVarId b1, ConstraintVarId b2,
                            ConstraintVarId reif) = 0;

  virtual void bool_lin_eq(const std::vector<Int>& coeffs,
                           const std::vector<ConstraintVarId>& inputs, Int rhs,
                           bool shouldHold) = 0;

  virtual void bool_lin_eq(const std::vector<Int>& coeffs,
                           const std::vector<ConstraintVarId>& inputs,
                           ConstraintVarId rhs,
                           Int rhsOffset,
                           bool shouldHold) = 0;

  virtual void bool_lin_eq_reif(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                Int rhs, ConstraintVarId reif) = 0;

  virtual void bool_lin_le(const std::vector<Int>& coeffs,
                           const std::vector<ConstraintVarId>& inputs, Int rhs,
                           bool shouldHold) = 0;

  virtual void bool_lin_le_reif(const std::vector<Int>& coeffs,
                                const std::vector<ConstraintVarId>& inputs,
                                Int rhs, ConstraintVarId reif) = 0;

  virtual void bool_lt(ConstraintVarId b1, ConstraintVarId b2,
                       bool shouldHold) = 0;

  virtual void bool_lt_reif(ConstraintVarId b1, ConstraintVarId b2,
                            ConstraintVarId reif) = 0;

  virtual void bool_not(ConstraintVarId b1, ConstraintVarId b2,
                        bool shouldHold) = 0;

  virtual void bool_not_reif(ConstraintVarId b1, ConstraintVarId b2,
                             ConstraintVarId reif) = 0;

  virtual void bool_or(ConstraintVarId b1, ConstraintVarId b2,
                       bool shouldHold) = 0;

  virtual void bool_or_reif(ConstraintVarId b1, ConstraintVarId b2,
                            ConstraintVarId reif) = 0;

  virtual void bool_xor(ConstraintVarId b1, ConstraintVarId b2,
                        bool shouldHold) = 0;

  virtual void bool_xor_reif(ConstraintVarId b1, ConstraintVarId b2,
                             ConstraintVarId reif) = 0;
};

}  // namespace atlantis::invariantgraph
