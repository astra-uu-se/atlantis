#include "maps.hpp"

/*******************VARIABLEMAP****************************/
Variable* VariableMap::add(std::shared_ptr<Variable> variable) {
  if (!exists(variable->getName())) {
    _variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
        variable->getName(), variable));
    _variableVector.push_back(variable.get());
  }
  return find(variable->getName());
}
ArrayVariable* VariableMap::add(std::shared_ptr<ArrayVariable> variable) {
  if (!exists(variable->getName())) {
    _arrayVariables.insert(
        std::pair<std::string, std::shared_ptr<ArrayVariable>>(
            variable->getName(), variable));
    _arrayVector.push_back(variable.get());
  }
  return findArray(variable->getName());
}
Variable* VariableMap::find(const std::string name) const {
  assert(_variables.find(name) != _variables.end());
  return _variables.find(name)->second.get();
}
ArrayVariable* VariableMap::findArray(const std::string name) const {
  assert(_arrayVariables.find(name) != _arrayVariables.end());
  return _arrayVariables.find(name)->second.get();
}
bool VariableMap::exists(std::string name) {
  return ((_variables.find(name) != _variables.end()) ||
          (_arrayVariables.find(name) != _arrayVariables.end()));
}
std::vector<Variable*> VariableMap::variables() { return _variableVector; }
std::vector<ArrayVariable*> VariableMap::arrays() { return _arrayVector; }

void VariableMap::reportDomainChange() { _changedDomain = true; }
void VariableMap::reportPotDefChange() { _changedPotDef = true; }

const std::vector<Variable*>& VariableMap::domSort() {
  if (_domainSorted.size() != _variableVector.size())
    _domainSorted = variables();
  if (_changedDomain) {
    std::sort(_domainSorted.begin(), _domainSorted.end(),
              Variable::compareDomain);
  }
  _changedDomain = false;
  return _domainSorted;
}

const std::vector<Variable*>& VariableMap::potDefSort() {
  if (_potDefSorted.size() != _variableVector.size())
    _potDefSorted = variables();
  if (_changedPotDef) {
    std::sort(_domainSorted.begin(), _domainSorted.end(),
              Variable::comparePotentialDefiners);
  }
  _changedPotDef = false;
  return _potDefSorted;
}

/****************************CONSTRAINTMAP**********************/
void ConstraintMap::add(std::shared_ptr<Constraint> constraint) {
  assert(!exists(constraint.get()));
  _constraints.insert(std::pair<Constraint*, std::shared_ptr<Constraint>>(
      constraint.get(), constraint));
  _constraintArray.push_back(constraint.get());
}
bool ConstraintMap::exists(Constraint* constraint) {
  return (_constraints.find(constraint) != _constraints.end());
}
void ConstraintMap::remove(Constraint* constraint) {
  assert(exists(constraint));
  _constraints.erase(constraint);
  auto it =
      std::find(_constraintArray.begin(), _constraintArray.end(), constraint);
  _constraintArray.erase(it);
}
const std::vector<Constraint*>& ConstraintMap::getVector() {
  return _constraintArray;
}
