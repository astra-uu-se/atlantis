#pragma once

#include "structure.hpp"

class VariableMap;

class Variable : public Node {
 public:
  Variable() = default;
  Variable(std::string name, std::vector<Annotation> annotations);
  virtual ~Variable() = default;
  virtual void init(VariableMap& variables) = 0;

  static bool compareDomain(Variable* v1, Variable* v2);

  std::string getLabel() override { return _name; };
  bool breakCycle() override { return false; };
  virtual void addConstraint(Node* constraint) = 0;
  virtual void removeConstraint(Node* constraint) = 0;
  virtual void defineBy(Constraint* constraint) = 0;
  virtual void removeDefinition() = 0;
  virtual void addPotentialDefiner(Constraint* constraint) = 0;
  virtual void removePotentialDefiner(Constraint* constraint) = 0;
  virtual bool isDefinable() = 0;
  virtual Int length() { return 1; }

  virtual Int domainSize() = 0;
  virtual Int upperBound() = 0;
  virtual Int lowerBound() = 0;
  virtual void imposeDomain(Domain* domain) = 0;
  virtual void unImposeDomain() = 0;
  bool hasImposedDomain() { return _hasImposedDomain; }

  virtual Int count() { return 0; };
  virtual Int definedCount() { return 0; };
  virtual bool isDefined() { return _isDefined; };
  virtual std::string getName() { return _name; };
  virtual Constraint* definedBy() { return _definedBy; };
  virtual std::set<Constraint*> potentialDefiners() {
    return _potentialDefiners;
  };

  bool _isDefined;

 protected:
  bool _hasImposedDomain = false;
  std::string _name;
  Constraint* _definedBy;
  std::vector<Annotation> _annotations;
  std::set<Node*> _nextConstraints;
  std::set<Constraint*> _potentialDefiners;
};

class SingleVariable : public Variable {
 public:
  SingleVariable() = default;
  SingleVariable(std::string name, std::vector<Annotation> annotations,
                 std::shared_ptr<Domain> domain)
      : Variable(name, annotations) {
    _domain = domain;
    _imposedDomain = domain.get();
  };
  void init(VariableMap& variables) override{};

  std::set<Node*> getNext() override { return _nextConstraints; };
  void addConstraint(Node* constraint) override;
  void removeConstraint(Node* constraint) override;
  void defineBy(Constraint* constraint) override;
  void removeDefinition() override;
  void addPotentialDefiner(Constraint* constraint) override;
  void removePotentialDefiner(Constraint* constraint) override;
  Int count() override { return 1; };
  bool isDefinable() override { return !_isDefined; };
  void imposeDomain(Domain* domain) override;
  void unImposeDomain() override;
  Int domainSize() override;
  Int lowerBound() override;
  Int upperBound() override;
  Int definedCount() override { return _isDefined ? 1 : 0; }

 private:
  Domain* _imposedDomain;
  std::shared_ptr<Domain> _domain;
};

class ArrayVariable : public Variable {
 public:
  ArrayVariable(std::string name, std::vector<Annotation> annotations,
                std::vector<Expression> expressions)
      : Variable(name, annotations) {
    _expressions = expressions;
  };
  ArrayVariable(std::vector<Variable*> elements);
  void init(VariableMap& variables) override;

  std::set<Node*> getNext() override;
  void addConstraint(Node* constraint) override;
  void removeConstraint(Node* constraint) override;
  void defineBy(Constraint* constraint) override;
  void defineNotDefinedBy(Constraint* constraint);
  void removeDefinition() override;
  void addPotentialDefiner(Constraint* constraint) override;
  void removePotentialDefiner(Constraint* constraint) override;
  std::vector<Variable*> elements();
  bool isDefinable() override;
  std::string getLabel() override;
  void imposeDomain(Domain* domain) override;
  void unImposeDomain() override;
  Int domainSize() override;
  Int lowerBound() override;
  Int upperBound() override;
  Int length() override;
  Variable* getElement(Int n);
  Int definedCount() override;

 private:
  std::vector<Expression> _expressions;
  std::vector<Variable*> _elements;
};

class Literal : public SingleVariable {
 public:
  Literal(std::string value);
  void init(VariableMap& variables) override;

  std::set<Node*> getNext() override;
  bool isDefinable() override { return false; };
  void addConstraint(Node* constraint) override{};
  void removeConstraint(Node* constraint) override{};
  void defineBy(Constraint* constraint) override{};
  void removeDefinition() override{};
  void addPotentialDefiner(Constraint* constraint) override{};
  void removePotentialDefiner(Constraint* constraint) override{};
  Int count() override { return 0; };
  void imposeDomain(Domain* domain) override{};
  void unImposeDomain() override{};
  Int domainSize() override { return 0; }
  Int lowerBound() override;
  Int upperBound() override;
};
class Parameter : public Literal {
 public:
  Parameter(std::string name, std::string value) : Literal(name) {
    _value = value;
  };
  Int lowerBound() override;
  Int upperBound() override;

 private:
  std::string _value;
};
