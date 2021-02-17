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
  ConstraintBox();
  ConstraintBox(std::string name, std::vector<Expression> expressions,
                 std::vector<Annotation> annotations);
  void prepare(std::map<std::string, std::shared_ptr<Variable>>& variables);
  std::string _name;
  std::vector<Expression> _expressions;
  std::vector<Annotation> _annotations;
};

class Constraint : public Node {
 public:
  Constraint();
  virtual ~Constraint() = default;
  Constraint(ConstraintBox constraintBox);
  virtual void init(const std::map<std::string, std::shared_ptr<Variable>>& variables) = 0;
  virtual std::vector<Node*> getNext() override;
  std::string getLabel() override;

  void defineVariable(Variable* variable);
  void unDefineVariable(Variable* variable);
  void addDependency(Variable* variable);
  void removeDependency(Variable* variable);
  Expression getExpression(int n);
  ArrayVariable* getArrayVariable(
      std::map<std::string, std::shared_ptr<Variable>> variables, int n);
  SingleVariable* getSingleVariable(
      std::map<std::string, std::shared_ptr<Variable>> variables, int n);
  std::string _name;
  ConstraintBox _constraintBox;
  std::vector<Node*> _defines;
};

/* global_cardinality(       array [int] of var int: x,
**                           array [int] of int: cover,
**                           array [int] of var int: counts)
** Defines: all of counts
** Depends: x
*/
class GlobalCardinality : public Constraint {
 public:
  GlobalCardinality(ConstraintBox constraintBox)
      : Constraint(constraintBox){};
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables) override;
  ArrayVariable* _x;
  SingleVariable* _cover;
  ArrayVariable* _counts;
};

/* int_div(var int: a, var int: b, var int: c)
** Defines: a
** Depends: b, c
*/
class IntDiv : public Constraint {
 public:
  IntDiv(ConstraintBox constraintBox) : Constraint(constraintBox){};
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables) override;
  SingleVariable* _a;
  SingleVariable* _b;
  SingleVariable* _c;
};
/* int_max(var int: a, var int: b, var int: c)
** Defines: c
** Depends: a, b
*/
class IntMax : public Constraint {
 public:
  IntMax(ConstraintBox constraintBox) : Constraint(constraintBox){};
  void init(const std::map<std::string, std::shared_ptr<Variable>>&
  variables)
      override;
  Variable* _a;
  Variable* _b;
  Variable* _c;
};
/* int_plus(var int: a, var int: b, var int: c)
** Defines: any
*/
class IntPlus : public Constraint {
 public:
  IntPlus(ConstraintBox constraintBox) : Constraint(constraintBox){};
  void init(const std::map<std::string, std::shared_ptr<Variable>>&
  variables)
      override;
  SingleVariable* _a;
  SingleVariable* _b;
  SingleVariable* _c;
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
class IntLinEq : public Constraint {
 public:
  IntLinEq(ConstraintBox constraintBox) : Constraint(constraintBox){};
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables) override;
  ArrayVariable* _as;
  ArrayVariable* _bs;
  SingleVariable* _c;
};
