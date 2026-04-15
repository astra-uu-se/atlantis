#pragma once

#include "atlantis/utils/gecode_compat.hpp"
#include <gecode/int.hh>
#include <gecode/kernel.hh>
#include <vector>

#include "varNode.hpp"

namespace atlantis::invariantgraph {

class ConstraintSolver : public Gecode::Space {
  /// Index of the variable to optimize
  std::optional<size_t> _optVar;
  /// Whether variable to optimize is integer (or float)
  bool _optVarIsInt;

  /// Copy constructor
  ConstraintSolver(ConstraintSolver&);


  /// The integer variables
  std::vector<Gecode::IntVar> _iv;
  /// The Boolean variables
  std::vector<Gecode::BoolVar> _bv;

  Gecode::BoolVarArgs boolVarArgs(const std::vector<ConstraintVarId>&);
  Gecode::BoolVar boolVar(ConstraintVarId);

  Gecode::IntVarArgs intVarArgs(const std::vector<ConstraintVarId>&);
  Gecode::IntVar intVar(ConstraintVarId);

  void array_bool_rel(const std::vector<ConstraintVarId>& inputs, ConstraintVarId reified, Gecode::BoolOpType);
  void array_bool_rel(const std::vector<ConstraintVarId>& inputs, bool shouldHold, Gecode::BoolOpType);

  public:
    /// Construct empty space
    ConstraintSolver();

    /// Destructor
    ~ConstraintSolver() override = default;

    [[nodiscard]] size_t numIntVars() const noexcept { return _iv.size(); }
    [[nodiscard]] size_t numBoolVars() const noexcept { return _bv.size(); }

    ConstraintVarId newIntVar(Int);
    ConstraintVarId newIntVar(const SearchDomain&);

    ConstraintVarId newBoolVar(bool);
    ConstraintVarId newBoolVar();
    void setOptVar(const VarNode&, bool isIntVar = true);
    void fixPoint();

    /// Return index of variable used for optimization
    [[nodiscard]] std::optional<size_t> optVar() const { return _optVar; }
    /// Return whether variable used for optimization is integer (or Boolean)
    [[nodiscard]] bool optVarIsInt() const { return _optVarIsInt; }

    [[nodiscard]] SearchDomain intVarDomain(ConstraintVarId) const;
    [[nodiscard]] SearchDomain boolVarDomain(ConstraintVarId) const;

    void constrain(const Space&) override {};
    /// Copy function
    Gecode::Space* copy() override;


    // Constraints
    void array_bool_and(const std::vector<ConstraintVarId>& inputs, ConstraintVarId reified);
    void array_bool_and(const std::vector<ConstraintVarId>& inputs, bool shouldHold);
    void array_bool_or(const std::vector<ConstraintVarId>& inputs,
                       ConstraintVarId reified);
    void array_bool_or(const std::vector<ConstraintVarId>& inputs,
                       bool shouldHold);
    void array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                        ConstraintVarId reified);
    void array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                        bool shouldHold);
    void bool2int(ConstraintVarId boolVarId, ConstraintVarId intVarId);
  void array_bool_element(const ConstraintVarId& index,
                            const std::vector<Int>& parameters,
                            ConstraintVarId output,
                            Int offset);
  void array_bool_element2d(const ConstraintVarId& index1,
                            const ConstraintVarId& index2,
                            const std::vector<std::vector<Int>>& parameters,
                            ConstraintVarId output,
                            Int rowOffset,
                            Int colOffset);

};

}  // namespace atlantis::invariantgraph
