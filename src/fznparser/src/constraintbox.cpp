#include "constraintbox.hpp"

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
