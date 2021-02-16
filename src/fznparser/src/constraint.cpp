#include "constraint.hpp"

/********************* Constraint Item *********************/
ConstraintItem::ConstraintItem() {}
ConstraintItem::ConstraintItem(std::string name,
                               std::vector<Expression> expressions,
                               std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}
void ConstraintItem::init(std::map<std::string, std::shared_ptr<Item>>& items) {
  for (auto e : _expressions) {
    if (items.find(e.getName()) == items.end()) {
      if (e.isArray()) {  // Create new entry for literal array.
        std::vector<Annotation> ann;
        auto av =
            std::make_shared<ArrayVariable>(e.getName(), ann, e._elements);
        av->init(items);
        items.insert(std::pair<std::string, std::shared_ptr<Item>>(
            av->getName(), static_cast<std::shared_ptr<Item>>(av)));
      } else if (!e.isId()) {

        auto p = std::make_shared<Parameter>(e.getName());
        items.insert(std::pair<std::string, std::shared_ptr<Item>>(
            p->getName(), static_cast<std::shared_ptr<Item>>(p)));
      }
    }
  }
}

/********************* Constraint **************************/
Constraint::Constraint() {}
Constraint::Constraint(ConstraintItem constraintItem) {
  _constraintItem = constraintItem;
  _name = constraintItem._name;
}

Expression Constraint::getExpression(int n) {
  assert(n < _constraintItem._expressions.size());
  return _constraintItem._expressions[n];
}

std::vector<Node*> Constraint::getNext() { return _defines; }
std::string Constraint::getLabel() { return _name; }

SingleVariable* Constraint::getSingleVariable(
    std::map<std::string, std::shared_ptr<Item>> items, int n) {
  std::string name = getExpression(n).getName();

  assert(items.find(name) != items.end());
  Item* s = items.find(name)->second.get();
  return dynamic_cast<SingleVariable*>(s);
}
ArrayVariable* Constraint::getArrayVariable(
    std::map<std::string, std::shared_ptr<Item>> items, int n) {
  std::string name = getExpression(n).getName();
  assert(items.find(name) != items.end());
  Item* s = items.find(name)->second.get();
  return dynamic_cast<ArrayVariable*>(s);
}

void Constraint::defineVariable(Variable* variable) {
  _defines.push_back(variable);
  variable->defineBy(this);
}

/********************* GlobalCardinality ******************************/
void GlobalCardinality::init(
    std::map<std::string, std::shared_ptr<Item>>& items) {
  // [a, b, 3] -> [a, b]
  _x = getArrayVariable(items, 0);
  _cover = getSingleVariable(items, 1);
  _counts = getArrayVariable(items, 2);

  _x->addConstraint(this);
  defineVariable(_counts);

  // for (auto v : _x) {
  //   v->addConstraint(this);
  // }
  // for (auto v : _counts) {
  //   defineVariable(v);
  // }
  // Skriv om till count och en mindre version av sig själv?
  // Har vi en cykel och kan den undvikas?
}
/********************* IntDiv ******************************/
void IntDiv::init(std::map<std::string, std::shared_ptr<Item>>& items) {
  _a = getSingleVariable(items, 0);
  _b = getSingleVariable(items, 1);
  _c = getSingleVariable(items, 2);

  defineVariable(_a);
  _b->addConstraint(this);
  _c->addConstraint(this);
}

// /********************* IntMax ******************************/
// void IntMax::init(
//     const std::map<std::string, std::shared_ptr<Variable>>& variables) {
//   _a = getVariable(variables, _constraintItem._expressions[0]._name);
//   _b = getVariable(variables, _constraintItem._expressions[1]._name);
//   _c = getVariable(variables, _constraintItem._expressions[2]._name);

//   defineVariable(_c);
//   _a->addConstraint(this);
//   _b->addConstraint(this);
// }

// /********************* IntPlus ******************************/
// void IntPlus::init(
//     const std::map<std::string, std::shared_ptr<Variable>>& variables) {
//   _a = getVariable(variables, _constraintItem._expressions[0]._name);
//   _b = getVariable(variables, _constraintItem._expressions[1]._name);
//   _c = getVariable(variables, _constraintItem._expressions[2]._name);

//   defineVariable(_a);
//   _b->addConstraint(this);
//   _c->addConstraint(this);
// }
