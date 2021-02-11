#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cassert>

#include "structure.hpp"

class ConstraintItem {
 public:
  ConstraintItem();
  ConstraintItem(std::string name, std::vector<Expression> expressions,
                 std::vector<Annotation> annotations);
  std::string _name;
  std::vector<Expression> _expressions;
  std::vector<Annotation> _annotations;
};

class Constraint : public Node {
 public:
  Constraint();
  virtual void init(
      const std::map<std::string, std::shared_ptr<Variable>>& variables) = 0;
  virtual ~Constraint() = default;
  virtual std::vector<Node*> getNext();

 protected:
  void defineVariable(Variable* variable);
  virtual Variable* getVariable(
      const std::map<std::string, std::shared_ptr<Variable>>& variables,
      std::string name);
  ConstraintItem _constraintItem;
  std::string _name;
  std::vector<Node*> _next;
};

/* int_div(var int: a, var int: b, var int: c)
** Defines: a
** Depends: b, c
*/
class IntDiv : public Constraint {
 public:
  IntDiv(ConstraintItem constraintItem);
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables)
      override;
  Variable* _a;
  Variable* _b;
  Variable* _c;
};
/* int_max(var int: a, var int: b, var int: c)
** Defines: c
** Depends: a, b
*/
class IntMax : public Constraint {
 public:
  IntMax(ConstraintItem constraintItem);
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables)
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
  IntPlus(ConstraintItem constraintItem);
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables)
      override;
  Variable* _a;
  Variable* _b;
  Variable* _c;
};
/* array_var_int_element(var int: b, array [int] of var int: as, var int: c)
** Defines: c
** Depends: b (statically), as (dynamically)
*/
class ArrayVarIntElement : public Constraint {
 public:
  ArrayVarIntElement(ConstraintItem constraintItem);
  void init(const std::map<std::string, std::shared_ptr<Variable>>& variables)
      override;
};
/* global_cardinality(array [int] of var int: x,
**                           array [int] of int: cover,
**                           array [int] of var int: counts)
** Defines: all of counts
** Depends: x
*/
