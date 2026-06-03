#pragma once

#include <vector>

#include "atlantis/types.hpp"
#include "atlantis/utils/domains.hpp"
#include "atlantis/invariantgraph/types.hpp"

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
                           const std::vector<ConstraintVarId>& inputs,
                           ConstraintVarId rhs, Int rhsOffset) = 0;

  virtual void bool_lin(const std::vector<Int>& coeffs,
                        const std::vector<ConstraintVarId>& inputs,
                        RelationType, Int rhs, bool shouldHold) = 0;

  virtual void bool_lin_reif(const std::vector<Int>& coeffs,
                             const std::vector<ConstraintVarId>& inputs,
                             RelationType, Int rhs, ConstraintVarId reif) = 0;

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

  virtual void int_abs(ConstraintVarId lhs, ConstraintVarId rhs) = 0;

  virtual void int_div(ConstraintVarId numerator, ConstraintVarId denominator,
                       ConstraintVarId quotient) = 0;

  virtual void int_eq(ConstraintVarId lhs, ConstraintVarId rhs,
                      bool shouldHold) = 0;

  virtual void int_eq_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                           ConstraintVarId reified) = 0;

  virtual void int_eq_reif(ConstraintVarId lhs, Int rhs,
                           ConstraintVarId reified) = 0;

  virtual void int_le(ConstraintVarId lhs, ConstraintVarId rhs,
                      bool shouldHold) = 0;

  virtual void int_le_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                           ConstraintVarId reified) = 0;

  virtual void int_lin_eq(const std::vector<Int>& coeffs,
                          const std::vector<ConstraintVarId>& inputs,
                          ConstraintVarId rhs, Int rhsOffset) = 0;

  virtual void int_lin(const std::vector<Int>& coeffs,
                       const std::vector<ConstraintVarId>& inputs,
                       RelationType relationType, Int rhs, bool shouldHold) = 0;

  virtual void int_lin_reif(const std::vector<Int>& coeffs,
                            const std::vector<ConstraintVarId>& inputs,
                            RelationType relationType, Int rhs,
                            ConstraintVarId reified) = 0;

  virtual void int_lt(ConstraintVarId lhs, ConstraintVarId rhs,
                      bool shouldHold) = 0;

  virtual void int_lt_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                           ConstraintVarId reified) = 0;

  virtual void int_max(ConstraintVarId a, ConstraintVarId b,
                       ConstraintVarId maximum) = 0;

  virtual void int_min(ConstraintVarId a, ConstraintVarId b,
                       ConstraintVarId minimum) = 0;

  virtual void int_mod(ConstraintVarId numerator, ConstraintVarId denominator,
                       ConstraintVarId remainder) = 0;

  virtual void int_ne(ConstraintVarId lhs, ConstraintVarId rhs,
                      bool shouldHold) = 0;

  virtual void int_ne_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                           ConstraintVarId reified) = 0;

  virtual void int_plus(ConstraintVarId a, ConstraintVarId b,
                        ConstraintVarId sum) = 0;

  virtual void int_pow(ConstraintVarId base, ConstraintVarId exponent,
                       ConstraintVarId power) = 0;

  virtual void int_times(ConstraintVarId a, ConstraintVarId b,
                         ConstraintVarId product) = 0;

  virtual void fzn_all_different_int(const std::vector<ConstraintVarId>& inputs,
                                     bool shouldHold) = 0;

  virtual void fzn_all_different_int_reif(
      const std::vector<ConstraintVarId>& inputs, ConstraintVarId reified) = 0;

  virtual void fzn_all_equal_int(const std::vector<ConstraintVarId>& inputs,
                                 bool shouldHold) = 0;

  virtual void fzn_all_equal_int_reif(
      const std::vector<ConstraintVarId>& inputs, ConstraintVarId reified) = 0;

  virtual void fzn_circuit(const std::vector<ConstraintVarId>& inputs,
                           Int offset, bool shouldHold) = 0;

  virtual void fzn_circuit_reif(const std::vector<ConstraintVarId>& inputs,
                                Int offset, ConstraintVarId reified) = 0;

  virtual void fzn_global_cardinality(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<ConstraintVarId>& counts, bool shouldHold) = 0;

  virtual void fzn_global_cardinality_reif(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<ConstraintVarId>& counts, ConstraintVarId reified) = 0;

  virtual void fzn_global_cardinality_closed(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<ConstraintVarId>& counts, bool shouldHold) = 0;

  virtual void fzn_global_cardinality_closed_reif(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<ConstraintVarId>& counts, ConstraintVarId reified) = 0;

  virtual void fzn_global_cardinality_low_up(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
      bool shouldHold) = 0;

  virtual void fzn_global_cardinality_low_up_reif(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
      ConstraintVarId reified) = 0;

  virtual void fzn_global_cardinality_low_up_closed(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
      bool shouldHold) = 0;

  virtual void fzn_global_cardinality_low_up_closed_reif(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
      ConstraintVarId reified) = 0;

  virtual void fzn_count(const std::vector<ConstraintVarId>& inputs, Int needle,
                         RelationType relation, Int amount,
                         bool shouldHold) = 0;

  virtual void fzn_count(const std::vector<ConstraintVarId>& inputs, Int needle,
                         RelationType relation, ConstraintVarId amount,
                         bool shouldHold) = 0;

  virtual void fzn_count(const std::vector<ConstraintVarId>& inputs,
                         ConstraintVarId needle, RelationType relation,
                         Int amount, bool shouldHold) = 0;

  virtual void fzn_count(const std::vector<ConstraintVarId>& inputs,
                         ConstraintVarId needle, RelationType relation,
                         ConstraintVarId amount, bool shouldHold) = 0;

  virtual void fzn_count_reif(const std::vector<ConstraintVarId>& inputs,
                              Int needle, RelationType relation, Int amount,
                              ConstraintVarId reified) = 0;

  virtual void fzn_count_reif(const std::vector<ConstraintVarId>& inputs,
                              Int needle, RelationType relation,
                              ConstraintVarId amount,
                              ConstraintVarId reified) = 0;

  virtual void fzn_count_reif(const std::vector<ConstraintVarId>& inputs,
                              ConstraintVarId needle, RelationType relation,
                              Int amount, ConstraintVarId reified) = 0;

  virtual void fzn_count_reif(const std::vector<ConstraintVarId>& inputs,
                              ConstraintVarId needle, RelationType relation,
                              ConstraintVarId amount,
                              ConstraintVarId reified) = 0;

  virtual void nvalue(ConstraintVarId numVals,
                      const std::vector<ConstraintVarId>& inputs) = 0;

  virtual void nvalue_lt(Int numVals,
                         const std::vector<ConstraintVarId>& inputs) = 0;
};

}  // namespace atlantis::invariantgraph
