#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "structure.hpp"
#include "variable.hpp"

class ConstraintBox {
 public:
  ConstraintBox() = default;
  ConstraintBox(std::string name, std::vector<Expression> expressions,
                std::vector<Annotation> annotations);
  void prepare(std::map<std::string, std::shared_ptr<Variable>>& variables);
  bool hasDefineAnnotation();
  std::string getAnnotationVariableName();
  std::string _name;
  std::vector<Expression> _expressions;
  std::vector<Annotation> _annotations;
};

class Constraint : public Node {
 public:
  virtual ~Constraint() = default;
  Constraint(ConstraintBox constraintBox);
  virtual void init(
      const std::map<std::string, std::shared_ptr<Variable>>& variables) = 0;
  virtual std::set<Node*> getNext() override;
  std::string getLabel() override;
  virtual void tweak();
  virtual void makeOneWay() = 0;

  void clearVariables();
  void defineVariable(Variable* variable);
  void unDefineVariable(Variable* variable);
  void addDependency(Variable* variable);
  void removeDependency(Variable* variable);
  bool hasDefineAnnotation();
  Variable* annotationDefineVariable();
  Expression getExpression(int n);
  ArrayVariable* getArrayVariable(
      std::map<std::string, std::shared_ptr<Variable>> variables, int n);
  SingleVariable* getSingleVariable(
      std::map<std::string, std::shared_ptr<Variable>> variables, int n);
  Variable* getAnnotationVariable(
      std::map<std::string, std::shared_ptr<Variable>> variables);
  void defineByAnnotation();
  void forceOneWay(Variable* variable);

  std::string _name;
  ConstraintBox _constraintBox;
  std::set<Node*> _defines;
  std::vector<Variable*> _variables;
  bool _hasDefineAnnotation;
  Variable* _annotationDefineVariable;
};

class ThreeSVarConstraint : public Constraint {
 public:
  ThreeSVarConstraint(ConstraintBox constraintBox)
      : Constraint(constraintBox){};
  virtual void init(const std::map<std::string, std::shared_ptr<Variable>>&
                        variables) override;
  void makeOneWay() override;
  SingleVariable* _a;
  SingleVariable* _b;
  SingleVariable* _c;
};

/* global_cardinality(       array [int] of var int: x,
**                           array [int] of int: cover,
**                           array [int] of var int: counts)
** Defines: all of counts
** Depends: x
*/
class GlobalCardinality : public Constraint {
 public:
  GlobalCardinality(ConstraintBox constraintBox) : Constraint(constraintBox){};
  virtual void init(const std::map<std::string, std::shared_ptr<Variable>>&
                        variables) override;
  void makeOneWay() override;
  ArrayVariable* _x;
  SingleVariable* _cover;
  ArrayVariable* _counts;
};
/* int_div(var int: a, var int: b, var int: c)
** Defines: a
** Depends: b, c
*/
class IntDiv : public ThreeSVarConstraint {
 public:
  IntDiv(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox){};
  void makeOneWay() override;
};
/* int_plus(var int: a, var int: b, var int: c)
** Defines: any
*/
class IntPlus : public ThreeSVarConstraint {
 public:
  IntPlus(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox){};
  void tweak();
  int _state = 0;
};
/* array_var_int_element(var int: b, array [int] of var int: as, var int: c)
** Defines: c
** Depends: b (statically), as (dynamically)
*/
// class ArrayVarIntElement : public Constraint {
//  public:
//   ArrayVarIntElement(ConstraintBox constraintBox)
//       : Constraint(constraintBox){};
//   void init(const std::map<std::string, std::shared_ptr<Variable>>&
//   variables)
//       override;
// };
// class IntLinEq : public Constraint {
//  public:
//   IntLinEq(ConstraintBox constraintBox) : Constraint(constraintBox){};
//   void init(const std::map<std::string, std::shared_ptr<Variable>>&
//   variables)
//       override;
//   void defineTarget() override;

//   ArrayVariable* _as;
//   ArrayVariable* _bs;
//   SingleVariable* _c;
// };

/* int_max(var int: a, var int: b, var int: c)
** Defines: c
** Depends: a, b
*/
// class IntMax : public ThreeSVarConstraint {
//  public:
//   IntMax(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox){};
// };
