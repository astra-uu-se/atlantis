#pragma once

#include "../constraint.hpp"

class ReifiedConstraint : public Constraint {
 public:
  ReifiedConstraint(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void configureVariables() override;
};

class ReifiedConstraintSingle : public ReifiedConstraint {
 public:
  ReifiedConstraintSingle(ConstraintBox constraintBox)
      : ReifiedConstraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
};
class ReifiedConstraintArray : public ReifiedConstraint {
 public:
  ReifiedConstraintArray(ConstraintBox constraintBox)
      : ReifiedConstraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
};

class IntEqReif : public ReifiedConstraintSingle {  // 2
 public:
  IntEqReif(ConstraintBox constraintBox)
      : ReifiedConstraintSingle(constraintBox) {}
};
class IntLeReif : public ReifiedConstraintSingle {  // 2
 public:
  IntLeReif(ConstraintBox constraintBox)
      : ReifiedConstraintSingle(constraintBox) {}
};
class IntLinEqReif : public ReifiedConstraintArray {  // 3
 public:
  IntLinEqReif(ConstraintBox constraintBox)
      : ReifiedConstraintArray(constraintBox) {}
};
class IntLinLeReif : public ReifiedConstraintArray {  // 3
 public:
  IntLinLeReif(ConstraintBox constraintBox)
      : ReifiedConstraintArray(constraintBox) {}
};
class IntLinNeReif : public ReifiedConstraintArray {  // 3
 public:
  IntLinNeReif(ConstraintBox constraintBox)
      : ReifiedConstraintArray(constraintBox) {}
};
class IntLtReif : public ReifiedConstraintSingle {  // 2
 public:
  IntLtReif(ConstraintBox constraintBox)
      : ReifiedConstraintSingle(constraintBox) {}
};
class IntNeReif : public ReifiedConstraintSingle {  // 2
 public:
  IntNeReif(ConstraintBox constraintBox)
      : ReifiedConstraintSingle(constraintBox) {}
};
class BoolEqReif : public ReifiedConstraintSingle {  // 2
 public:
  BoolEqReif(ConstraintBox constraintBox)
      : ReifiedConstraintSingle(constraintBox) {}
};
class BoolLeReif : public ReifiedConstraintSingle {  // 2
 public:
  BoolLeReif(ConstraintBox constraintBox)
      : ReifiedConstraintSingle(constraintBox) {}
};
class BoolLtReif : public ReifiedConstraintSingle {
 public:
  BoolLtReif(ConstraintBox constraintBox)
      : ReifiedConstraintSingle(constraintBox) {}
};
