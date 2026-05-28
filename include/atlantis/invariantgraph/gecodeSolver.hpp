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

#include "constraintSolver.hpp"
#include "varNode.hpp"

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
  Gecode::IntVar intVar(size_t);
  Gecode::IntVar intVar(ConstraintVarId);

  static Gecode::IntSharedArray intSharedArray(const std::vector<Int>&);
  static Gecode::IntSharedArray intSharedArray(const std::vector<bool>&);

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

  void bool_rel(ConstraintVarId lhs, ConstraintVarId rhs,
                ConstraintVarId reified, Gecode::IntRelType);
  void bool_rel(ConstraintVarId lhs, ConstraintVarId rhs, bool shouldHold,
                Gecode::IntRelType);

  void int_rel(ConstraintVarId lhs, ConstraintVarId rhs,
               ConstraintVarId reified, Gecode::IntRelType irt);
  void int_rel(ConstraintVarId lhs, Int rhs, ConstraintVarId reified,
               Gecode::IntRelType irt);
  void int_rel(ConstraintVarId lhs, ConstraintVarId rhs, bool shouldHold,
               Gecode::IntRelType irt);

  void bool_lin_rel(const std::vector<Int>& coeffs,
                    const std::vector<ConstraintVarId>& inputs, Int rhs,
                    ConstraintVarId reified, Gecode::IntRelType);
  void bool_lin_rel(const std::vector<Int>& coeffs,
                    const std::vector<ConstraintVarId>& inputs,
                    ConstraintVarId rhs, Int rhsOffset, Gecode::IntRelType irt);
  void bool_lin_rel(const std::vector<Int>& coeffs,
                    const std::vector<ConstraintVarId>& inputs, Int rhs,
                    bool shouldHold, Gecode::IntRelType);

  void int_lin_rel(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs, Int rhs,
                   ConstraintVarId reified, Gecode::IntRelType);
  void int_lin_rel(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs,
                   ConstraintVarId rhs, Int rhsOffset, Gecode::IntRelType irt);
  void int_lin_rel(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs, Int rhs,
                   bool shouldHold, Gecode::IntRelType);

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

  void bool_eq(ConstraintVarId b1, ConstraintVarId b2,
               bool shouldHold) override;

  void bool_eq_reif(ConstraintVarId b1, ConstraintVarId b2,
                    ConstraintVarId reified) override;

  void bool_le(ConstraintVarId b1, ConstraintVarId b2,
               bool shouldHold) override;

  void bool_le_reif(ConstraintVarId b1, ConstraintVarId b2,
                    ConstraintVarId reified) override;

  void bool_lin_eq(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs, Int rhs,
                   bool shouldHold) override;

  void bool_lin_eq(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs,
                   ConstraintVarId rhs, Int rhsOffset) override;

  void bool_lin_eq_reif(const std::vector<Int>& coeffs,
                        const std::vector<ConstraintVarId>& inputs, Int rhs,
                        ConstraintVarId reified) override;

  void bool_lin_le(const std::vector<Int>& coeffs,
                   const std::vector<ConstraintVarId>& inputs, Int rhs,
                   bool shouldHold) override;

  void bool_lin_le_reif(const std::vector<Int>& coeffs,
                        const std::vector<ConstraintVarId>& inputs, Int rhs,
                        ConstraintVarId reified) override;

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

  void int_le_reif(ConstraintVarId lhs, ConstraintVarId rhs,
                   ConstraintVarId reified) override;

  void int_lin_eq(const std::vector<Int>& coeffs,
                  const std::vector<ConstraintVarId>& inputs, Int rhs,
                  bool shouldHold) override;

  void int_lin_eq(const std::vector<Int>& coeffs,
                  const std::vector<ConstraintVarId>& inputs,
                  ConstraintVarId rhs, Int rhsOffset) override;

  void int_lin_eq_reif(const std::vector<Int>& coeffs,
                       const std::vector<ConstraintVarId>& inputs, Int rhs,
                       ConstraintVarId shouldHold) override;

  void int_lin_le(const std::vector<Int>& coeffs,
                  const std::vector<ConstraintVarId>& inputs, Int rhs,
                  bool shouldHold) override;

  void int_lin_le_reif(const std::vector<Int>& coeffs,
                       const std::vector<ConstraintVarId>& inputs, Int rhs,
                       ConstraintVarId reified) override;

  void int_lin_ne(const std::vector<Int>& coeffs,
                  const std::vector<ConstraintVarId>& inputs, Int rhs,
                  bool shouldHold) override;

  void int_lin_ne_reif(const std::vector<Int>& coeffs,
                       const std::vector<ConstraintVarId>& inputs, Int rhs,
                       ConstraintVarId reified) override;

  void int_lt(ConstraintVarId lhs, ConstraintVarId rhs,
              bool shouldHold) override;

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

  void nvalue(ConstraintVarId numVals,
              const std::vector<ConstraintVarId>& inputs) override;

  void nvalue_lt(Int numVals,
                 const std::vector<ConstraintVarId>& inputs) override;
};

}  // namespace atlantis::invariantgraph
