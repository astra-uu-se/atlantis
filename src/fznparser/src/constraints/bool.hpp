#pragma once

#include "../constraint.hpp"
class BoolArrayConstraint : public Constraint {
 public:
  BoolArrayConstraint(ConstraintBox constraintBox)
      : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  ArrayVariable* _as;
};
class ArrayBoolAnd : public BoolArrayConstraint {
 public:
  ArrayBoolAnd(ConstraintBox constraintBox)
      : BoolArrayConstraint(constraintBox) {}
};
class ArrayBoolOr : public BoolArrayConstraint {
 public:
  ArrayBoolOr(ConstraintBox constraintBox)
      : BoolArrayConstraint(constraintBox) {}
};

class BoolOneToOne : public Constraint {
 public:
  BoolOneToOne(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
};

class Bool2Int : public BoolOneToOne {
 public:
  Bool2Int(ConstraintBox constraintBox) : BoolOneToOne(constraintBox) {}
};
class BoolEq : public BoolOneToOne {
 public:
  BoolEq(ConstraintBox constraintBox) : BoolOneToOne(constraintBox) {}
};
class BoolNot : public BoolOneToOne {
 public:
  BoolNot(ConstraintBox constraintBox) : BoolOneToOne(constraintBox) {}
};

class BoolThreeVar : public Constraint {
 public:
  BoolThreeVar(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
};

class BoolAnd : public BoolThreeVar {
 public:
  BoolAnd(ConstraintBox constraintBox) : BoolThreeVar(constraintBox) {}
};
class BoolOr : public BoolThreeVar {
 public:
  BoolOr(ConstraintBox constraintBox) : BoolThreeVar(constraintBox) {}
};
class BoolXor : public BoolThreeVar {
 public:
  BoolXor(ConstraintBox constraintBox) : BoolThreeVar(constraintBox) {}
};

class BoolLinEq : public SimpleConstraint {
 public:
  BoolLinEq(ConstraintBox constraintBox) : SimpleConstraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
  ArrayVariable* _as;
  ArrayVariable* _bs;
};
