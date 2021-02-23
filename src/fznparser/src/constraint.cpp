#include "constraint.hpp"

/********************* ConstraintBox *********************/
ConstraintBox::ConstraintBox(std::string name,
                             std::vector<Expression> expressions,
                             std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}
void ConstraintBox::prepare(
    std::map<std::string, std::shared_ptr<Variable>>& variables) {
  for (auto e : _expressions) {
    if (variables.find(e.getName()) == variables.end()) {
      if (e.isArray()) {  // Create new entry for literal array.
        std::vector<Annotation> ann;
        auto av =
            std::make_shared<ArrayVariable>(e.getName(), ann, e._elements);
        variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
            av->getName(), static_cast<std::shared_ptr<Variable>>(av)));
      } else if (!e.isId()) {
        auto p = std::make_shared<Parameter>(e.getName());
        variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
            p->getName(), static_cast<std::shared_ptr<Variable>>(p)));
      }
    }
  }
}

bool ConstraintBox::hasDefineAnnotation() {
  for (auto a : _annotations) {
    if (a.definesVar()) {
      return true;
    }
  }
  return false;
}
std::string ConstraintBox::getAnnotationVariableName() {
  for (auto annotation : _annotations) {
    if (annotation._definesVar) {
      return annotation._variableName;
    }
  }
  return "";
}

/********************* Constraint **************************/
Constraint::Constraint(ConstraintBox constraintBox) {
  _constraintBox = constraintBox;
  _name = constraintBox._name;
  _uniqueTarget = true;
}

Expression Constraint::getExpression(int n) {
  assert(n < _constraintBox._expressions.size());
  return _constraintBox._expressions[n];
}

std::set<Node*> Constraint::getNext() {
  std::set<Node*> defines;
  for (auto var : _defines) {
    defines.insert(var);
  }
  return defines;
}
std::string Constraint::getLabel() { return _name; }

bool Constraint::breakCycle() {
  makeSoft();
  return true;
}

SingleVariable* Constraint::getSingleVariable(
    std::map<std::string, std::shared_ptr<Variable>> variables, int n) {
  std::string name = getExpression(n).getName();

  assert(variables.find(name) != variables.end());
  Variable* s = variables.find(name)->second.get();
  return dynamic_cast<SingleVariable*>(s);
}
ArrayVariable* Constraint::getArrayVariable(
    std::map<std::string, std::shared_ptr<Variable>> variables, int n) {
  std::string name = getExpression(n).getName();
  assert(variables.find(name) != variables.end());
  Variable* s = variables.find(name)->second.get();
  return dynamic_cast<ArrayVariable*>(s);
}
Variable* Constraint::getAnnotationVariable(
    std::map<std::string, std::shared_ptr<Variable>> variables) {
  std::string name = _constraintBox.getAnnotationVariableName();

  assert(variables.find(name) != variables.end());
  return variables.find(name)->second.get();
}

void Constraint::defineVariable(Variable* variable) {
  _defines.insert(variable);
  variable->defineBy(this);
}
void Constraint::unDefineVariable(Variable* variable) {
  assert(variable->_definedBy == this);
  _defines.erase(variable);
  variable->removeDefinition();
}
void Constraint::addDependency(Variable* variable) {
  variable->addConstraint(this);
}
void Constraint::removeDependency(Variable* variable) {
  variable->removeConstraint(this);
}
void Constraint::clearVariables() {
  for (auto v : _variables) {
    if (v->_definedBy == this) {
      unDefineVariable(v);
    }
    removeDependency(v);
  }
}
bool Constraint::hasDefineAnnotation() { return _hasDefineAnnotation; }
void Constraint::defineByAnnotation() {
  defineVariable(_annotationDefineVariable);
  for (auto variable : _variables) {
    if (variable != _annotationDefineVariable) {
      addDependency(variable);
    }
  }
}
void Constraint::makeOneWay(Variable* variable) {
  clearVariables();
  defineVariable(variable);
  for (auto v : _variables) {
    if (variable != v) {
      addDependency(v);
    }
  }
}
void Constraint::makeSoft() {
  // Remove dependencies?? Maybe just trigger a bool isSoft instead? and check
  // it in getNext()
  for (auto var : _defines) {
    var->removeDefinition();
  }
  _defines.clear();
}
/********************* ThreeSVarConstraint ******************************/
void ThreeSVarConstraint::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getSingleVariable(variables, 0);
  _b = getSingleVariable(variables, 1);
  _c = getSingleVariable(variables, 2);

  _variables.push_back(_a);
  _variables.push_back(_b);
  _variables.push_back(_c);
  configureVariables();

  if (_constraintBox.hasDefineAnnotation()) {
    _annotationDefineVariable = getAnnotationVariable(variables);
    _hasDefineAnnotation = true;
  }
}
void ThreeSVarConstraint::configureVariables() {
  _c->addPotentialDefiner(this);
}
/********************* IntDiv ******************************/
void IntDiv::configureVariables() { _a->addPotentialDefiner(this); }
/********************* IntPlus ******************************/
void IntPlus::configureVariables() {
  _a->addPotentialDefiner(this);
  _b->addPotentialDefiner(this);
  _c->addPotentialDefiner(this);
}
/********************* GlobalCardinality ******************************/
void GlobalCardinality::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _x = getArrayVariable(variables, 0);
  _cover = getArrayVariable(variables, 1);  // Parameter
  _counts = getArrayVariable(variables, 2);
  _variables.push_back(_x);
  _variables.push_back(_counts);
  _counts->addPotentialDefiner(this);
}
/********************* IntLinEq ******************************/
