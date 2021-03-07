#pragma once
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "constraintbox.hpp"
#include "maps.hpp"
#include "structure.hpp"
#include "variable.hpp"

class Constraint : public Node {
 public:
  virtual ~Constraint() = default;
  Constraint();
  Constraint(ConstraintBox constraintBox);
  void init(const VariableMap& variables);
  std::string getName();

  std::set<Node*> getNext() override;
  std::string getLabel() override;
  bool breakCycle() override;

  void makeOneWayByAnnotation();
  void makeOneWay(Variable* variable);
  virtual bool canBeOneWay(Variable* variable) { return true; }
  void makeSoft();
  bool definesNone();
  bool uniqueTarget();
  bool canDefineByAnnotation();
  virtual bool canBeImplicit();
  virtual void makeImplicit();
  std::vector<Variable*> variables();
  std::vector<Variable*> variablesSorted();
  virtual bool split(int index, VariableMap& variables,
                     ConstraintMap& constraints) {
    return false;
  }

  bool isImplicit() { return _implicit; }
  bool isInvariant() { return _invariant; }

 protected:
  virtual void loadVariables(const VariableMap& variables) = 0;
  virtual void configureVariables() = 0;
  ArrayVariable* getArrayVariable(const VariableMap& variables, int n);
  SingleVariable* getSingleVariable(const VariableMap& variables, int n);
  Variable* getAnnotationVariable(const VariableMap& variables);
  Expression getExpression(int n);
  Variable* annotationDefineVariable();
  void checkAnnotations(const VariableMap& variables);

  void defineVariable(Variable* variable);
  void unDefineVariable(Variable* variable);
  void addDependency(Variable* variable);
  void removeDependency(Variable* variable);
  void clearVariables();
  void cleanse();

  virtual void imposeDomain(Variable* variable) {}

  std::shared_ptr<Domain> _imposedDomain;
  std::string _name;
  ConstraintBox _constraintBox;
  std::set<Variable*> _defines;
  std::vector<Variable*> _variables;
  bool _hasDefineAnnotation;
  Variable* _annotationDefineVariable;
  bool _uniqueTarget;
  bool _implicit = false;
  bool _invariant = false;
};

class NonFunctionalConstraint : public Constraint {
 public:
  NonFunctionalConstraint(ConstraintBox constraintBox)
      : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override {}
  void configureVariables() override {}
};

class ThreeSVarConstraint : public Constraint {
 public:
  ThreeSVarConstraint(ConstraintBox constraintBox)
      : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
};
/* int_div(var int: a, var int: b, var int: c)
** Defines: a
** Depends: b, c
*/
class IntDiv : public ThreeSVarConstraint {
 public:
  IntDiv(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {}
  void configureVariables() override;
  void imposeDomain(Variable* variable) override;
};
/* int_plus(var int: a, var int: b, var int: c)
** Defines: any
*/
class IntPlus : public ThreeSVarConstraint {
 public:
  IntPlus(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {
    _uniqueTarget = false;
  }
  void imposeDomain(Variable* variable) override;
  void configureVariables() override;
};
class TwoSVarConstraint : public Constraint {
 public:
  TwoSVarConstraint(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
};
class IntAbs : public TwoSVarConstraint {
 public:
  IntAbs(ConstraintBox constraintBox) : TwoSVarConstraint(constraintBox) {}
};

/* global_cardinality(       array [int] of var int: x,
**                           array [int] of int: cover,
**                           array [int] of var int: counts)
** Defines: all of counts
** Depends: x
*/
class GlobalCardinality : public Constraint {
 public:
  GlobalCardinality(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  GlobalCardinality(ArrayVariable* x, ArrayVariable* cover,
                    ArrayVariable* counts);
  bool split(int index, VariableMap& variables,
             ConstraintMap& constraints) override;
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeOneWay(Variable* variable) override;
  bool canBeImplicit() override;
  void makeImplicit() override;

 private:
  ArrayVariable* _x;
  ArrayVariable* _cover;  // const array
  ArrayVariable* _counts;
};

class IntLinEq : public Constraint {
 public:
  IntLinEq(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;

 private:
  ArrayVariable* _as;
  ArrayVariable* _bs;
  SingleVariable* _c;
};
class AllDifferent : public Constraint {
 public:
  AllDifferent(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;

 private:
  ArrayVariable* _x;
};
class Inverse : public Constraint {
 public:
  Inverse(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;
};

class Element : public Constraint {
 public:
  Element(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
};
class Circuit : public Constraint {
 public:
  Circuit(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;

 private:
  ArrayVariable* _x;
};
