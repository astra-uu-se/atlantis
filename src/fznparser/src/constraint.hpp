#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "constraintbox.hpp"
#include "structure.hpp"
#include "variable.hpp"

class Constraint : public Node {
 public:
  virtual ~Constraint() = default;
  Constraint(ConstraintBox constraintBox);
  virtual void init(
      const std::map<std::string, std::shared_ptr<Variable>>& variables) = 0;

  virtual std::set<Node*> getNext() override;
  std::string getLabel() override;
  virtual bool breakCycle() override;

  void makeOneWayByAnnotation();
  void makeOneWay(Variable* variable);
  void makeSoft();
  virtual void makeImplicit();
  virtual bool canBeImplicit();
  bool definesNone();
  bool uniqueTarget();
  bool hasDefineAnnotation();
  std::vector<Variable*> variables();

 protected:
  ArrayVariable* getArrayVariable(
      std::map<std::string, std::shared_ptr<Variable>> variables, int n);
  SingleVariable* getSingleVariable(
      std::map<std::string, std::shared_ptr<Variable>> variables, int n);
  Variable* getAnnotationVariable(
      std::map<std::string, std::shared_ptr<Variable>> variables);
  Expression getExpression(int n);
  Variable* annotationDefineVariable();
  void checkAnnotations(
      const std::map<std::string, std::shared_ptr<Variable>>& variables);

  void defineVariable(Variable* variable);
  void unDefineVariable(Variable* variable);
  void addDependency(Variable* variable);
  void removeDependency(Variable* variable);
  void clearVariables();

  std::string _name;
  ConstraintBox _constraintBox;
  std::set<Variable*> _defines;
  std::vector<Variable*> _variables;
  bool _hasDefineAnnotation;
  Variable* _annotationDefineVariable;
  bool _uniqueTarget;
  bool _implicit;
  bool _canBeImplicit;
};

class ThreeSVarConstraint : public Constraint {
 public:
  ThreeSVarConstraint(ConstraintBox constraintBox)
      : Constraint(constraintBox){};
  virtual void init(const std::map<std::string, std::shared_ptr<Variable>>&
                        variables) override;
  virtual void configureVariables();
  SingleVariable* _a;
  SingleVariable* _b;
  SingleVariable* _c;
};
/* int_div(var int: a, var int: b, var int: c)
** Defines: a
** Depends: b, c
*/
class IntDiv : public ThreeSVarConstraint {
 public:
  IntDiv(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox){};
  void configureVariables() override;
};
/* int_plus(var int: a, var int: b, var int: c)
** Defines: any
*/
class IntPlus : public ThreeSVarConstraint {
 public:
  IntPlus(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {
    _uniqueTarget = false;
  };
  void configureVariables() override;
};
class TwoSVarConstraint : public Constraint {
 public:
  TwoSVarConstraint(ConstraintBox constraintBox) : Constraint(constraintBox){};
  virtual void init(const std::map<std::string, std::shared_ptr<Variable>>&
                        variables) override;
  virtual void configureVariables();
  SingleVariable* _a;
  SingleVariable* _b;
};
class IntAbs : public TwoSVarConstraint {
 public:
  IntAbs(ConstraintBox constraintBox) : TwoSVarConstraint(constraintBox){};
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
  };
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables)
      override;
  ArrayVariable* _x;
  ArrayVariable* _cover;
  ArrayVariable* _counts;
};

class IntLinEq : public Constraint {
 public:
  IntLinEq(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  };
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables)
      override;
  ArrayVariable* _as;
  ArrayVariable* _bs;
  SingleVariable* _c;
};
class AllDifferent : public Constraint {
 public:
  AllDifferent(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  };
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables)
      override;
  bool canBeImplicit() override;
  void makeImplicit() override;
  ArrayVariable* _x;
};
