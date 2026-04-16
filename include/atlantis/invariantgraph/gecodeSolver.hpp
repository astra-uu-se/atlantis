#pragma once

#pragma once

#ifndef _LIBCPP_STD_VER
#  if  __cplusplus <= 201103L
#    define _LIBCPP_STD_VER 11
#  elif __cplusplus <= 201402L
#    define _LIBCPP_STD_VER 14
#  elif __cplusplus <= 201703L
#    define _LIBCPP_STD_VER 17
#  elif __cplusplus <= 202002L
#    define _LIBCPP_STD_VER 20
#  elif __cplusplus <= 202302L
#    define _LIBCPP_STD_VER 23  // current year, or date of c++2a ratification
#  else
#    define _LIBCPP_STD_VER 26
#  endif
#endif

#include <gecode/int.hh>
#include <gecode/kernel.hh>
#include <vector>

#include "atlantis/utils/gecode_compat.hpp"
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
  Gecode::BoolVar& boolVar(size_t);
  Gecode::BoolVar& boolVar(ConstraintVarId);

  Gecode::IntVarArgs intVarArgs(const std::vector<ConstraintVarId>&);
  Gecode::IntVar& intVar(size_t);
  Gecode::IntVar& intVar(ConstraintVarId);

  void array_bool_rel(const std::vector<ConstraintVarId>& inputs, ConstraintVarId reified, Gecode::BoolOpType);
  void array_bool_rel(const std::vector<ConstraintVarId>& inputs, bool shouldHold, Gecode::BoolOpType);

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
    void array_bool_and(const std::vector<ConstraintVarId>& inputs, ConstraintVarId reified) override;
    void array_bool_and(const std::vector<ConstraintVarId>& inputs, bool shouldHold) override;
    void array_bool_or(const std::vector<ConstraintVarId>& inputs,
                       ConstraintVarId reified) override;
    void array_bool_or(const std::vector<ConstraintVarId>& inputs,
                       bool shouldHold) override;
    void array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                        ConstraintVarId reified) override;
    void array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                        bool shouldHold) override;
    void bool2int(ConstraintVarId boolVarId, ConstraintVarId intVarId) override;
    void array_bool_element(const ConstraintVarId& index,
                            const std::vector<Int>& parameters,
                            ConstraintVarId output,
                            Int offset) override;
    void array_bool_element2d(const ConstraintVarId& index1,
                            const ConstraintVarId& index2,
                            const std::vector<std::vector<Int>>& parameters,
                            ConstraintVarId output,
                            Int rowOffset,
                            Int colOffset) override;

};

}  // namespace atlantis::invariantgraph
