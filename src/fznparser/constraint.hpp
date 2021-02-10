#pragma once
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

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

class Constraint : public Node,  public std::enable_shared_from_this<Constraint>{
 public:
  Constraint();
  virtual void print() = 0;
  virtual void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) = 0;

  ConstraintItem _constraintItem;
  std::string _name;
};


/* int_div(var int: a, var int: b, var int: c)
** Defines: a
** Depends: b, c
*/
class IntDiv : public Constraint {
 public:
  IntDiv(ConstraintItem constraintItem);
  void print() override;
  void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) override;
  std::vector<std::shared_ptr<Node>> getNext() override;
  Variable* _a;
  Variable* _b;
  Variable* _c;
  std::vector<std::shared_ptr<Node>> _next;
};
/* array_var_int_element(var int: b, array [int] of var int: as, var int: c)
** Defines: c
** Depends: b (statically), as (dynamically)
*/
class ArrayVarIntElement : public Constraint {
 public:
  ArrayVarIntElement(ConstraintItem constraintItem);
  void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) override;
};
/* int_plus(var int: a, var int: b, var int: c)
** Defines: any
*/
class IntPlus : public Constraint {
 public:
  IntPlus(ConstraintItem constraintItem);
  void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) override;
};
/* int_max(var int: a, var int: b, var int: c)
** Defines: c
** Depends: a, b
 */
class IntMax : public Constraint {
 public:
  IntMax(ConstraintItem constraintItem);
  void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) override;
};
/* global_cardinality(array [int] of var int: x,
**                           array [int] of int: cover,
**                           array [int] of var int: counts)
** Defines: all of counts
** Depends: x
 */
