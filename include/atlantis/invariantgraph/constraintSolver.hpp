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
  std::vector<Gecode::IntVar> _bv;
  public:
    /// Construct empty space
    ConstraintSolver();

    /// Destructor
    ~ConstraintSolver() override = default;

    ConstraintVarId newIntVar(Int);
    ConstraintVarId newIntVar(const SearchDomain&);

    ConstraintVarId newBoolVar(bool);
    ConstraintVarId newBoolVar();
    void setOptVar(const VarNode&, bool isIntVar = true);

    /// Return index of variable used for optimization
    [[nodiscard]] std::optional<size_t> optVar() const { return _optVar; }
    /// Return whether variable used for optimization is integer (or Boolean)
    [[nodiscard]] bool optVarIsInt() const { return _optVarIsInt; }

    void constrain(const Space&) override {};
    /// Copy function
    Gecode::Space* copy() override;
};

}  // namespace atlantis::invariantgraph
