#pragma once

#ifndef _LIBCPP_STD_VER
#if __cplusplus <= 201103L
#define _LIBCPP_STD_VER 11
#elif __cplusplus <= 201402L
#define _LIBCPP_STD_VER 14
#elif __cplusplus <= 201703L
#define _LIBCPP_STD_VER 17
#elif __cplusplus <= 202002L
#define _LIBCPP_STD_VER 20
#elif __cplusplus <= 202302L
#define _LIBCPP_STD_VER 23  // current year, or date of c++2a ratification
#else
#define _LIBCPP_STD_VER 26
#endif
#endif

#include <gecode/int.hh>
#include <gecode/kernel.hh>
#include <vector>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/sortedUniqueVector.hpp"

namespace atlantis::invariantgraph {

class GecodeSolver : public ConstraintSolver {
  class GecodeSpace : public Gecode::Space {
    GecodeSpace(GecodeSpace&);

   public:
    /// The integer variables
    std::vector<Gecode::IntVar> _iv;
    /// The Boolean variables
    std::vector<Gecode::BoolVar> _bv;
    /// Copy constructor
    GecodeSpace() = default;
    ~GecodeSpace() override = default;
    void constrain(const Space&) override {}
    Space* copy() override;
  };

  GecodeSpace _space;

  Gecode::BoolVarArgs boolVarArgs(const std::vector<ConstraintVarId>&);
  Gecode::BoolVarArgs boolVarArgs(
      const std::vector<std::vector<ConstraintVarId>>&);
  Gecode::BoolVar boolVar(size_t);
  Gecode::BoolVar boolVar(ConstraintVarId);

  Gecode::IntVarArgs intVarArgs(const std::vector<ConstraintVarId>&);
  Gecode::IntVarArgs intVarArgs(
      const std::vector<std::vector<ConstraintVarId>>&);

  static Gecode::TupleSet tupleSet(const std::vector<std::vector<Int>>& table);
  static Gecode::TupleSet tupleSet(const std::vector<std::vector<bool>>& table);

  static Gecode::IntSet intSet(const SortedUniqueVector& values);

  Gecode::IntVar intVar(size_t);
  Gecode::IntVar intVar(ConstraintVarId);

  static Gecode::IntSharedArray intSharedArray(const std::vector<Int>&);
  static Gecode::IntSharedArray intSharedArray(const std::vector<bool>&);
  static Gecode::IntArgs intArgs(const std::vector<Int>&);
  static Gecode::IntArgs intArgs(const std::vector<bool>&);

  static Gecode::IntSharedArray intSharedArray(
      const std::vector<std::vector<Int>>&);
  static Gecode::IntSharedArray intSharedArray(
      const std::vector<std::vector<bool>>&);

  void array_bool_op(const std::vector<ConstraintVarId>& inputs,
                     ConstraintVarId reified, Gecode::BoolOpType);
  void array_bool_op(const std::vector<ConstraintVarId>& inputs,
                     bool shouldHold, Gecode::BoolOpType);

  void bool_op(ConstraintVarId lhs, ConstraintVarId rhs,
               ConstraintVarId reified, Gecode::BoolOpType);
  void bool_op(ConstraintVarId lhs, ConstraintVarId rhs, bool shouldHold,
               Gecode::BoolOpType);

  void bool_lin_rel(const std::vector<Int>& coeffs,
                    const std::vector<ConstraintVarId>& inputs, RelationType,
                    Int rhs, ConstraintVarId reified);
  void bool_lin_rel(const std::vector<Int>& coeffs,
                    const std::vector<ConstraintVarId>& inputs, RelationType,
                    ConstraintVarId rhs, Int rhsOffset);
  void bool_lin_rel(const std::vector<Int>& coeffs,
                    const std::vector<ConstraintVarId>& inputs, RelationType,
                    Int rhs, bool shouldHold);

  void int_lin_rel(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs, RelationType,
                   Int rhs, ConstraintVarId reified);
  void int_lin_rel(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs, RelationType,
                   ConstraintVarId rhs, Int rhsOffset);
  void int_lin_rel(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs, RelationType,
                   Int rhs, bool shouldHold);

  Gecode::IntArgs gcc_get_cover(const Gecode::IntVarArgs& inputVars,
                                const std::vector<Int>& cover,
                                Gecode::IntVarArgs& countVars);

  bool allFixed(const std::vector<ConstraintVarId>& vars);

  bool isFixedTo(ConstraintVarId b, bool val);

  bool isFixedTo(std::variant<bool, ConstraintVarId> b, bool val);
  bool gcc_cover_sanity(const std::vector<Int>& cover,
                        std::variant<bool, ConstraintVarId> reified);

  bool gcc_sanity(const std::vector<ConstraintVarId>& inputs,
                  const std::vector<Int>& cover,
                  const std::vector<ConstraintVarId>& counts,
                  std::variant<bool, ConstraintVarId> reified);
  bool gcc_closed_sanity(const std::vector<Int>& cover,
                         std::variant<bool, ConstraintVarId> reified);

  bool gcc_sanity(const std::vector<ConstraintVarId>& inputs,
                  const std::vector<Int>& cover,
                  const std::vector<Int>& lowerBounds,
                  const std::vector<Int>& upperBounds,
                  std::variant<bool, ConstraintVarId> reified);
  std::pair<std::vector<Int>, std::vector<ConstraintVarId>> gcc_combine_covers(
      const std::vector<Int>& cover, const std::vector<ConstraintVarId>& counts,
      std::variant<bool, ConstraintVarId> reified);

  static std::pair<std::vector<Int>,
                   std::pair<std::vector<Int>, std::vector<Int>>>
  gcc_combine_covers(const std::vector<Int>& cover,
                     const std::vector<Int>& lowerBounds,
                     const std::vector<Int>& upperBounds);

  void gcc(const std::vector<ConstraintVarId>& inputs,
           const std::vector<Int>& cover,
           const std::vector<ConstraintVarId>& counts,
           std::variant<bool, ConstraintVarId> reified);
  void gcc_closed(const std::vector<ConstraintVarId>& inputs,
                  const std::vector<Int>& cover,
                  const std::vector<ConstraintVarId>& counts,
                  std::variant<bool, ConstraintVarId> reified);

  void gcc_low_up(const std::vector<ConstraintVarId>& inputs,
                  const std::vector<Int>& cover,
                  const std::vector<Int>& lowerBounds,
                  const std::vector<Int>& upperBounds,
                  std::variant<bool, ConstraintVarId> reified);

  void gcc_low_up_closed(const std::vector<ConstraintVarId>& inputs,
                         const std::vector<Int>& cover,
                         const std::vector<Int>& lowerBounds,
                         const std::vector<Int>& upperBounds,
                         std::variant<bool, ConstraintVarId> reified);

 public:
  /// Construct empty space
  GecodeSolver();

  [[nodiscard]] size_t numIntVars() const override;
  [[nodiscard]] size_t numBoolVars() const override;

  ConstraintVarId newIntVar(Int) override;
  ConstraintVarId newIntVar(const SearchDomain&) override;

  ConstraintVarId newBoolVar(bool) override;
  ConstraintVarId newBoolVar() override;

  [[nodiscard]] SearchDomain intVarDomain(ConstraintVarId) const override;
  [[nodiscard]] SearchDomain boolVarDomain(ConstraintVarId) const override;

  [[nodiscard]] SearchDomain varDomain(ConstraintVarId) const override;

  void fixPoint() override;

  // Constraints
  void fix_bool(ConstraintVarId, bool) override;

  void fix_int(ConstraintVarId, Int) override;

  void array_bool_and(const std::vector<ConstraintVarId>& inputs,
                      ConstraintVarId reified) override;
  void array_bool_and(const std::vector<ConstraintVarId>& inputs,
                      bool shouldHold) override;
  void array_bool_element(ConstraintVarId index,
                          const std::vector<bool>& parameters,
                          ConstraintVarId output, Int offset) override;
  void array_bool_element2d(ConstraintVarId rowIndex, ConstraintVarId colIndex,
                            const std::vector<std::vector<bool>>& parameters,
                            ConstraintVarId output, Int rowOffset,
                            Int colOffset) override;
  void array_bool_or(const std::vector<ConstraintVarId>& inputs,
                     ConstraintVarId reified) override;
  void array_bool_or(const std::vector<ConstraintVarId>& inputs,
                     bool shouldHold) override;
  void array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                      ConstraintVarId reified) override;
  void array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                      bool shouldHold) override;

  void bool2int(ConstraintVarId boolVarId, ConstraintVarId intVarId) override;

  void array_int_element(ConstraintVarId index,
                         const std::vector<Int>& parameters,
                         ConstraintVarId output, Int offset) override;
  void array_int_element2d(ConstraintVarId rowIndex, ConstraintVarId colIndex,
                           const std::vector<std::vector<Int>>& parameters,
                           ConstraintVarId output, Int rowOffset,
                           Int colOffset) override;
  void array_int_maximum(const std::vector<ConstraintVarId>& inputs,
                         ConstraintVarId output) override;
  void array_int_minimum(const std::vector<ConstraintVarId>& inputs,
                         ConstraintVarId output) override;
  void array_var_bool_element(ConstraintVarId index,
                              const std::vector<ConstraintVarId>& inputs,
                              ConstraintVarId output, Int offset) override;
  void array_var_bool_element2d(
      ConstraintVarId rowIndex, ConstraintVarId colIndex,
      const std::vector<std::vector<ConstraintVarId>>& inputs,
      ConstraintVarId output, Int rowOffset, Int colOffset) override;
  void array_var_int_element(ConstraintVarId index,
                             const std::vector<ConstraintVarId>& inputs,
                             ConstraintVarId output, Int offset) override;
  void array_var_int_element2d(
      ConstraintVarId rowIndex, ConstraintVarId colIndex,
      const std::vector<std::vector<ConstraintVarId>>& inputs,
      ConstraintVarId output, Int rowOffset, Int colOffset) override;

  void bool_and(ConstraintVarId b1, ConstraintVarId b2,
                bool shouldHold) override;

  void bool_and_reif(ConstraintVarId b1, ConstraintVarId b2,
                     ConstraintVarId reified) override;

  void bool_clause(const std::vector<ConstraintVarId>& posInputs,
                   const std::vector<ConstraintVarId>& negInputs,
                   bool shouldHold) override;

  void bool_clause_reif(const std::vector<ConstraintVarId>& posInputs,
                        const std::vector<ConstraintVarId>& negInputs,
                        ConstraintVarId reified) override;

  void bool_rel_reif(ConstraintVarId lhs, RelationType, ConstraintVarId rhs,
                     ConstraintVarId reified) override;
  void bool_rel(ConstraintVarId lhs, RelationType, ConstraintVarId rhs,
                bool shouldHold) override;

  void int_rel_reif(ConstraintVarId lhs, RelationType, ConstraintVarId rhs,
                    ConstraintVarId reified) override;
  void int_rel_reif(ConstraintVarId lhs, RelationType, Int rhs,
                    ConstraintVarId reified) override;
  void int_rel(ConstraintVarId lhs, RelationType, ConstraintVarId rhs,
               bool shouldHold) override;

  void bool_eq(ConstraintVarId b1, ConstraintVarId b2,
               bool shouldHold) override;

  void bool_eq_reif(ConstraintVarId b1, ConstraintVarId b2,
                    ConstraintVarId reified) override;

  void bool_le(ConstraintVarId b1, ConstraintVarId b2,
               bool shouldHold) override;

  void bool_le_reif(ConstraintVarId b1, ConstraintVarId b2,
                    ConstraintVarId reified) override;

  void bool_lin_eq(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs,
                   ConstraintVarId rhs, Int rhsOffset) override;

  void bool_lin(const std::vector<Int>& coeffs,
                const std::vector<ConstraintVarId>& inputs, RelationType,
                Int rhs, bool shouldHold) override;

  void bool_lin_reif(const std::vector<Int>& coeffs,
                     const std::vector<ConstraintVarId>& inputs, RelationType,
                     Int rhs, ConstraintVarId reified) override;

  void bool_lt(ConstraintVarId b1, ConstraintVarId b2,
               bool shouldHold) override;

  void bool_lt_reif(ConstraintVarId b1, ConstraintVarId b2,
                    ConstraintVarId reified) override;

  void bool_not(ConstraintVarId b1, ConstraintVarId b2,
                bool shouldHold) override;

  void bool_not_reif(ConstraintVarId b1, ConstraintVarId b2,
                     ConstraintVarId reified) override;

  void bool_or(ConstraintVarId b1, ConstraintVarId b2,
               bool shouldHold) override;

  void bool_or_reif(ConstraintVarId b1, ConstraintVarId b2,
                    ConstraintVarId reified) override;

  void bool_xor(ConstraintVarId b1, ConstraintVarId b2,
                bool shouldHold) override;

  void bool_xor_reif(ConstraintVarId b1, ConstraintVarId b2,
                     ConstraintVarId reified) override;

  void int_abs(ConstraintVarId lhs, ConstraintVarId rhs) override;

  void int_div(ConstraintVarId numerator, ConstraintVarId denominator,
               ConstraintVarId quotient) override;

  void int_eq(ConstraintVarId lhs, ConstraintVarId rhs,
              bool shouldHold) override;

  void int_eq_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                   ConstraintVarId reified) override;

  void int_eq_reif(ConstraintVarId lhs, Int rhs,
                   ConstraintVarId reified) override;

  void int_le(ConstraintVarId lhs, ConstraintVarId rhs,
              bool shouldHold) override;

  void int_le_reif(ConstraintVarId lhs, Int rhs,
                   ConstraintVarId reified) override;

  void int_le_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                   ConstraintVarId reified) override;

  void int_lin_eq(const std::vector<Int>& coeffs,
                  const std::vector<ConstraintVarId>& inputs,
                  ConstraintVarId rhs, Int rhsOffset) override;

  void int_lin(const std::vector<Int>& coeffs,
               const std::vector<ConstraintVarId>& inputs,
               RelationType relation, Int rhs, bool shouldHold) override;

  void int_lin_reif(const std::vector<Int>& coeffs,
                    const std::vector<ConstraintVarId>& inputs,
                    RelationType relationType, Int rhs,
                    ConstraintVarId reified) override;

  void int_lt(ConstraintVarId lhs, ConstraintVarId rhs,
              bool shouldHold) override;

  void int_lt_reif(ConstraintVarId lhs, Int rhs,
                   ConstraintVarId reified) override;

  void int_lt_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                   ConstraintVarId reified) override;

  void int_max(ConstraintVarId a, ConstraintVarId b,
               ConstraintVarId maximum) override;

  void int_min(ConstraintVarId a, ConstraintVarId b,
               ConstraintVarId minimum) override;

  void int_mod(ConstraintVarId numerator, ConstraintVarId denominator,
               ConstraintVarId remainder) override;

  void int_ne(ConstraintVarId lhs, ConstraintVarId rhs,
              bool shouldHold) override;

  void int_ne_reif(ConstraintVarId lhs, Int rhs,
                   ConstraintVarId reified) override;

  void int_ne_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                   ConstraintVarId reified) override;

  void int_plus(ConstraintVarId a, ConstraintVarId b,
                ConstraintVarId sum) override;

  void int_pow(ConstraintVarId base, ConstraintVarId exponent,
               ConstraintVarId power) override;

  void int_times(ConstraintVarId a, ConstraintVarId b,
                 ConstraintVarId product) override;

  void fzn_all_different_int(const std::vector<ConstraintVarId>& inputs,
                             bool shouldHold) override;

  void fzn_all_different_int_reif(const std::vector<ConstraintVarId>& inputs,
                                  ConstraintVarId reified) override;

  void fzn_all_equal_int(const std::vector<ConstraintVarId>& inputs,
                         bool shouldHold) override;

  void fzn_all_equal_int_reif(const std::vector<ConstraintVarId>& inputs,
                              ConstraintVarId reified) override;

  void fzn_circuit(const std::vector<ConstraintVarId>& inputs, Int offset,
                   bool shouldHold) override;

  void fzn_circuit_reif(const std::vector<ConstraintVarId>& inputs, Int offset,
                        ConstraintVarId reified) override;

  void fzn_global_cardinality(const std::vector<ConstraintVarId>& inputs,
                              const std::vector<Int>& cover,
                              const std::vector<ConstraintVarId>& counts,
                              bool shouldHold) override;

  void fzn_global_cardinality_reif(const std::vector<ConstraintVarId>& inputs,
                                   const std::vector<Int>& cover,
                                   const std::vector<ConstraintVarId>& counts,
                                   ConstraintVarId reified) override;

  void fzn_global_cardinality_closed(const std::vector<ConstraintVarId>& inputs,
                                     const std::vector<Int>& cover,
                                     const std::vector<ConstraintVarId>& counts,
                                     bool shouldHold) override;

  void fzn_global_cardinality_closed_reif(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<ConstraintVarId>& counts,
      ConstraintVarId reified) override;

  void fzn_global_cardinality_low_up(const std::vector<ConstraintVarId>& inputs,
                                     const std::vector<Int>& cover,
                                     const std::vector<Int>& lowerBounds,
                                     const std::vector<Int>& upperBounds,
                                     bool shouldHold) override;

  void fzn_global_cardinality_low_up_reif(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
      ConstraintVarId reified) override;

  void fzn_global_cardinality_low_up_closed(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
      bool shouldHold) override;

  void fzn_global_cardinality_low_up_closed_reif(
      const std::vector<ConstraintVarId>& inputs, const std::vector<Int>& cover,
      const std::vector<Int>& lowerBounds, const std::vector<Int>& upperBounds,
      ConstraintVarId reified) override;

  void fzn_count(Int bound, RelationType relation,
                 const std::vector<ConstraintVarId>& inputs, Int needle,
                 bool shouldHold) override;

  void fzn_count(ConstraintVarId bound, RelationType relation,
                 const std::vector<ConstraintVarId>& inputs, Int needle,
                 bool shouldHold) override;

  void fzn_count(Int bound, RelationType relation,
                 const std::vector<ConstraintVarId>& inputs,
                 ConstraintVarId needle, bool shouldHold) override;

  void fzn_count(ConstraintVarId bound, RelationType relation,
                 const std::vector<ConstraintVarId>& inputs,
                 ConstraintVarId needle, bool shouldHold) override;

  void fzn_count_reif(Int bound, RelationType relation,
                      const std::vector<ConstraintVarId>& inputs, Int needle,
                      ConstraintVarId reified) override;

  void fzn_count_reif(ConstraintVarId bound, RelationType relation,
                      const std::vector<ConstraintVarId>& inputs, Int needle,
                      ConstraintVarId reified) override;

  void fzn_count_reif(Int bound, RelationType relation,
                      const std::vector<ConstraintVarId>& inputs,
                      ConstraintVarId needle, ConstraintVarId reified) override;

  void fzn_count_reif(ConstraintVarId bound, RelationType relation,
                      const std::vector<ConstraintVarId>& inputs,
                      ConstraintVarId needle, ConstraintVarId reified) override;

  void fzn_table_bool(const std::vector<ConstraintVarId>& inputs,
                      const std::vector<std::vector<bool>>& table,
                      bool shouldHold) override;

  void fzn_table_bool_reif(const std::vector<ConstraintVarId>& inputs,
                           const std::vector<std::vector<Int>>& table,
                           ConstraintVarId reified) override;

  void fzn_table_int(const std::vector<ConstraintVarId>& inputs,
                     const std::vector<std::vector<Int>>& table,
                     bool shouldHold) override;

  void fzn_table_int_reif(const std::vector<ConstraintVarId>& inputs,
                          const std::vector<std::vector<Int>>& table,
                          ConstraintVarId reified) override;

  void set_in(ConstraintVarId varId, const SortedUniqueVector& values,
              bool shouldHold) override;

  void set_in(ConstraintVarId varId, Int lowerBound, Int upperBound,
              bool shouldHold) override;

  void set_in_reif(ConstraintVarId varId, const SortedUniqueVector& values,
                   ConstraintVarId reified) override;

  void set_in_reif(ConstraintVarId varId, Int lowerBound, Int upperBound,
                   ConstraintVarId reified) override;

  void nvalue(ConstraintVarId numVals,
              const std::vector<ConstraintVarId>& inputs) override;

  void nvalue_lt(Int numVals,
                 const std::vector<ConstraintVarId>& inputs) override;
};

}  // namespace atlantis::invariantgraph
