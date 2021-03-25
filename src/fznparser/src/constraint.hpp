#ifndef __CONSTRAINT_HPP_INCLUDED__
#define __CONSTRAINT_HPP_INCLUDED__

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "structure.hpp"
class Variable;
class ArrayVariable;
class ConstraintBox;
class VariableMap;
class ConstraintMap;

class ConstraintBox {
 public:
  ConstraintBox() = default;
  ConstraintBox(std::string name, std::vector<Expression> expressions,
                std::vector<Annotation> annotations);
  void prepare(VariableMap& variables);
  bool hasDefineAnnotation();
  bool hasImplicitAnnotation();
  bool hasIgnoreCycleAnnotation();
  std::string getAnnotationVariableName();
  void setId(Int id);
  Int _id;
  std::string _name;
  std::vector<Expression> _expressions;
  std::vector<Annotation> _annotations;
};

class Constraint : public Node {
 public:
  virtual ~Constraint() = default;
  Constraint();
  Constraint(ConstraintBox constraintBox);
  virtual void init(const VariableMap& variables);
  std::string getName() override;

  static bool sort(Constraint* c1, Constraint* c2);

  std::set<Node*> getNext() override;
  bool breakCycle();
  bool breakCycleWithBan();

  std::optional<Variable*> annotationTarget();
  virtual void unDefine(Variable* variable) { makeSoft(); }
  virtual void define(Variable* variable);
  virtual bool canDefine(Variable* variable) { return _canBeOneWay; }
  bool defines(Variable* variable);
  void makeSoft();
  bool definesNone();
  bool uniqueTarget();
  virtual bool canBeImplicit();
  bool shouldBeImplicit();
  virtual void makeImplicit();
  std::vector<Variable*> variables();
  std::vector<Variable*> variablesSorted();

  bool isImplicit() { return _implicit; }
  bool isInvariant() { return _invariant; }
  Int defInVarCount();

  virtual bool allVariablesFree();
  virtual bool imposeDomain(Variable* variable) { return false; }
  virtual bool refreshDomain() { return false; }
  void refreshNext(std::set<Constraint*>& visisted);
  virtual void imposeAndPropagate(Variable* variable);
  virtual void refreshAndPropagate(std::set<Constraint*>& visisted);

 protected:
  virtual void loadVariables(const VariableMap& variables) = 0;
  virtual void configureVariables() = 0;
  ArrayVariable* getArrayVariable(const VariableMap& variables, Int n);
  Variable* getSingleVariable(const VariableMap& variables, Int n);
  Variable* getAnnotationVariable(const VariableMap& variables);
  Expression getExpression(Int n);
  virtual void checkAnnotations(const VariableMap& variables);

  void redefineVariable(Variable* variable);
  void defineVariable(Variable* variable);
  void unDefineVariable(Variable* variable);
  void addDependency(Variable* variable);
  void removeDependency(Variable* variable);
  virtual void clearVariables();
  void removeDependencies();
  void removeDefinitions();
  void cleanse();

  std::string _name;
  ConstraintBox _constraintBox;
  std::set<Variable*> _defines;
  std::vector<Variable*> _variables;
  std::optional<Variable*> _annotationTarget;
  bool _shouldBeImplicit = false;
  bool _uniqueTarget = true;
  bool _implicit = false;
  bool _invariant = false;
  bool _canBeOneWay = true;
};

class NonFunctionalConstraint : public Constraint {
 public:
  NonFunctionalConstraint(ConstraintBox constraintBox)
      : Constraint(constraintBox) {
    bool _canBeOneWay = false;
  }
  void init(const VariableMap& variables) override {}
  void loadVariables(const VariableMap& variables) override {}
  void configureVariables() override {}
};

#include "constraints/alldifferent.hpp"
#include "constraints/circuit.hpp"
#include "constraints/element.hpp"
#include "constraints/global_cardinality.hpp"
#include "constraints/int_lin_eq.hpp"
#include "constraints/integer_simple.hpp"
#include "constraints/inverse.hpp"
#include "variable.hpp"

#endif
