#include "constraintbox.hpp"

ConstraintBox::ConstraintBox(std::string name,
                             std::vector<Expression> expressions,
                             std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}
void ConstraintBox::prepare(VariableMap& variables) {
  for (auto e : _expressions) {
    if (!variables.exists(e.getName())) {
      if (e.isArray()) {  // Create new entry for literal array.
        std::vector<Annotation> ann;
        auto av =
            std::make_shared<ArrayVariable>(e.getName(), ann, e._elements);
        variables.add(av);
      } else if (!e.isId()) {
        auto p = std::make_shared<Literal>(e.getName());
        variables.add(p);
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
