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
  virtual void array_bool_element(const ConstraintVarId& index,
                                  const std::vector<Int>& parameters,
                                  ConstraintVarId output, Int offset) = 0;
  virtual void array_bool_element2d(
      const ConstraintVarId& index1, const ConstraintVarId& index2,
      const std::vector<std::vector<Int>>& parameters, ConstraintVarId output,
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
  virtual void array_int_element(const ConstraintVarId& index,
                                 const std::vector<Int>& parameters,
                                 ConstraintVarId output, Int offset) = 0;
  virtual void array_int_element2d(
      const ConstraintVarId& index1, const ConstraintVarId& index2,
      const std::vector<std::vector<Int>>& parameters, ConstraintVarId output,
      Int rowOffset, Int colOffset) = 0;
  virtual void array_int_maximum(
      const std::vector<ConstraintVarId>& inputs, ConstraintVarId output) = 0;
  virtual void array_int_minimum(
      const std::vector<ConstraintVarId>& inputs, ConstraintVarId output) = 0;
};

}  // namespace atlantis::invariantgraph
