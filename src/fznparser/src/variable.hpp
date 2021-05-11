#pragma once

class VariableMap;
#include "structure.hpp"

class Variable : public Node {
 public:
  Variable() = default;
  Variable(std::string name, std::vector<Annotation> annotations,
           std::shared_ptr<Domain> domain)
      : _name{name}, _annotations{annotations}, _domain{domain} {};

  static bool compareDomain(Variable* v1, Variable* v2);
  static bool comparePotentialDefiners(Variable* v1, Variable* v2);

  std::string getName() override { return _name; }
  std::vector<Node*> getNext() override;
  virtual std::vector<Constraint*> getNextConstraint();

  void addNextConstraint(Constraint* constraint);
  void removeNextConstraint(Constraint* constraint);
  void defineBy(Constraint* constraint);
  void removeDefinition();

  void imposeDomain(Domain* domain);
  void unImposeDomain();
  bool hasImposedDomain() { return _imposedDomain.has_value(); }
  bool hasEnlargedDomain();

  Constraint* definedBy() { return _definedBy.value(); }
  bool isDefined() { return _definedBy.has_value(); }

  void addPotentialDefiner(Constraint* constraint);
  void removePotentialDefiner(Constraint* constraint);
  bool hasPotentialDefiner(Constraint* constraint);
  std::vector<Constraint*> potentialDefiners();

  virtual bool isDefinable() { return true; }
  virtual Int domainSize();
  virtual Int lowerBound();
  virtual Int upperBound();
  void reset();
  void clearPotentialDefiners() { _potentialDefiners.clear(); }
  Int orgPotDefSize() { return _orgPotentialDefiners.size(); }
  bool noPotDef() { return _orgPotentialDefiners.size() == 0; }

  std::optional<Domain*> _imposedDomain;
  const std::shared_ptr<Domain> _domain;

 private:
  std::optional<Constraint*> _definedBy;
  std::set<Constraint*> _nextConstraints;
  std::set<Constraint*> _potentialDefiners;
  std::set<Constraint*> _orgPotentialDefiners;
  const std::string _name;
  std::vector<Annotation> _annotations;
};

class Literal : public Variable {
 public:
  Literal() = default;
  Literal(std::string value) : _valuename{value} {};

  Int lowerBound() override;
  Int upperBound() override;
  Int domainSize() override { return 1; }
  bool isDefinable() override { return false; }
  std::string getName() override { return _valuename; }

 protected:
  std::string _valuename;
};
class Parameter : public Literal {
 public:
  Parameter(std::string name, std::string value) {
    _pname = name;
    _value = value;
  };
  Int lowerBound() override;
  Int upperBound() override;
  std::string getName() override { return _pname; };

 private:
  std::string _pname;
  std::string _value;
};

class ArrayVariable {
 public:
  ArrayVariable(std::string name, std::vector<Annotation> annotations,
                std::vector<Expression> expressions)
      : _name{name}, _annotations{annotations}, _expressions{expressions} {}
  ArrayVariable(std::vector<Variable*> elements);
  void init(VariableMap& variables);

  std::vector<Variable*> elements();
  std::string getName() { return _name; }
  Variable* getElement(Int n);
  Int size() { return _elements.size(); }
  bool contains(Variable* variable);
  bool noneDefined();

 private:
  std::vector<Expression> _expressions;
  std::vector<Variable*> _elements;
  std::string _name;
  std::vector<Annotation> _annotations;
};

#include "constraint.hpp"
#include "maps.hpp"
