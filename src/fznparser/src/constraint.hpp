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
  virtual bool canBeOneWay(Variable* variable) { return _canBeOneWay; }
  void makeSoft();
  bool definesNone();
  bool uniqueTarget();
  bool canDefineByAnnotation();
  virtual bool canBeImplicit();
  bool shouldBeImplicit();
  virtual void makeImplicit();
  std::vector<Variable*> variables();
  std::vector<Variable*> variablesSorted();
  virtual bool split(int index, VariableMap& variables,
                     ConstraintMap& constraints) {
    return false;
  }

  bool isImplicit() { return _implicit; }
  bool isInvariant() { return _invariant; }
  Int defInVarCount();

  virtual bool imposeDomain(Variable* variable) { return false; }
  virtual bool refreshDomain() { return false; }
  void refreshNext(std::set<Constraint*>& visisted);
  virtual void imposeAndPropagate(Variable* variable);
  virtual void refreshAndPropagate(std::set<Constraint*>& visisted);

 protected:
  virtual void loadVariables(const VariableMap& variables) = 0;
  virtual void configureVariables() = 0;
  ArrayVariable* getArrayVariable(const VariableMap& variables, Int n);
  SingleVariable* getSingleVariable(const VariableMap& variables, Int n);
  Variable* getAnnotationVariable(const VariableMap& variables);
  Expression getExpression(Int n);
  Variable* annotationDefineVariable();
  void checkAnnotations(const VariableMap& variables);

  void defineVariable(Variable* variable);
  void unDefineVariable(Variable* variable);
  void addDependency(Variable* variable);
  void removeDependency(Variable* variable);
  void clearVariables();
  void cleanse();

  std::string _name;
  ConstraintBox _constraintBox;
  std::set<Variable*> _defines;
  std::vector<Variable*> _variables;
  bool _hasDefineAnnotation;
  bool _shouldBeImplicit = false;
  Variable* _annotationDefineVariable;
  bool _uniqueTarget;
  bool _implicit = false;
  bool _invariant = false;
  bool _canBeOneWay = true;
};

class NonFunctionalConstraint : public Constraint {
 public:
  NonFunctionalConstraint(ConstraintBox constraintBox)
      : Constraint(constraintBox) {}
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
