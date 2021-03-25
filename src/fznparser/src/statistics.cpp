#include "statistics.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

Statistics::Statistics(Model* model) { _model = model; }
int Statistics::variableCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    n++;
  }
  return n;
}
int Statistics::definedCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefined()) {
      n++;
    }
  }
  return n;
}
void Statistics::countDefinedVariables(bool labels) {
  std::cout << "===========================" << std::endl;
  std::cout << "=========VARIABLES=========" << std::endl;
  if (labels) {
    variablesDefinedBy();
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Defined:\t";
  std::cout << definedCount() << std::endl;
  std::cout << "Total:\t\t";
  std::cout << variableCount() << std::endl;
  std::cout << "===========================" << std::endl;
}
void Statistics::variablesDefinedBy() {
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefined()) {
      std::cout << "Var: " << variable->getName();
      std::cout << "\tDS: " << variable->domainSize();
      if (variable->hasImposedDomain()) {
        std::cout << "\t(imposed)";
      }
      std::cout << "\t[" << variable->definedBy()->getName() << "]"
                << std::endl;
    }
  }
}
int Statistics::countSoft() {
  int soft = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isImplicit() || constraint->isInvariant()) {
      continue;
    }
    soft++;
  }
  return soft;
}
void Statistics::constraints(bool labels) {
  std::cout << "========CONSTRAINTS========" << std::endl;
  int total = 0;
  int invariant = 0;
  int implicit = 0;
  for (auto constraint : _model->constraints()) {
    total++;
    if (constraint->isImplicit()) {
      implicit++;
    } else if (constraint->isInvariant()) {
      invariant++;
    }
    if (labels) {
      std::cout << constraint->getName() << std::endl;
    }
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Invariants:\t" << invariant << std::endl;
  std::cout << "Implicit:\t" << implicit << std::endl;
  std::cout << "Soft:\t\t" << total - invariant - implicit << std::endl;
  std::cout << "Total:\t\t" << total << std::endl;
  std::cout << "===========================" << std::endl;
}
void Statistics::allStats(bool labels) {
  countDefinedVariables(labels);
  constraints(labels);
  matchingAnnotations(labels);
  // width(labels);
}
void Statistics::matchingAnnotations(bool labels) {
  std::cout << "========ANNOTATIONS========" << std::endl;
  int annotations = 0;
  int matching = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->annotationTarget().has_value()) {
      annotations++;
      if (constraint->defines(constraint->annotationTarget().value())) {
        matching++;
        if (labels) {
          std::cout << constraint->getName() << " fulfills define annotation."
                    << std::endl;
        }
      }
    }
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Annotations:\t" << annotations << std::endl;
  std::cout << "Matching:\t" << matching << std::endl;
  std::cout << "===========================" << std::endl;
}
int Statistics::width(bool labels) {
  int w = 0;
  std::vector<Node*> visited;
  std::vector<Node*> result;
  int n = 0;
  for (Variable* v : _model->varMap().variables()) {
    if (!v->isDefined()) width_aux(result, visited, v, 1, w);
  }
  for (Constraint* c : _model->constraints()) {
    if (c->isImplicit()) width_aux(result, visited, c, 1, w);
  }
  if (labels) {
    std::cout << "Longest path: " << std::endl;
    for (auto n : result) {
      std::cout << n->getName() << std::endl;
    }
    std::cout << "===========================" << std::endl;
  }
  return w;
}
void Statistics::width_aux(std::vector<Node*>& result,
                           std::vector<Node*> visited, Node* node, int x,
                           int& w) {
  visited.push_back(node);
  if (x > w) {
    w = x;
    result = visited;
  }
  for (auto n : node->getNext()) {
    width_aux(result, visited, n, x + 1, w);
  }
}

int Statistics::annotationCount() {
  int annotations = 0;
  for (auto c : _model->constraints()) {
    if (c->annotationTarget().has_value()) {
      annotations++;
    }
  }
  return annotations;
}

std::optional<double> Statistics::matchingAnnotationsScore() {
  int annotations = 0;
  int matching = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->annotationTarget().has_value()) {
      annotations++;
      if (constraint->defines(constraint->annotationTarget().value())) {
        matching++;
      }
    }
  }
  if (annotations > 0) {
    return 100 * matching / (double)annotations;
  }
  return {};
}
std::optional<double> Statistics::variableScore() {
  int vc = variableCount();
  if (vc > 0) {
    return 100 * definedCount() / (double)vc;
  }
  return {};
}
std::optional<double> Statistics::constraintScore() {
  if (_model->constraints().size() > 0) {
    return 100 * (_model->constraints().size() - countSoft()) /
           (double)_model->constraints().size();
  }
  return {};
}
double Statistics::score() {
  int i = 0;
  double tot = 0;
  std::vector<std::optional<double>> scores;
  scores.push_back(matchingAnnotationsScore());
  scores.push_back(variableScore());
  scores.push_back(constraintScore());
  for (auto s : scores) {
    if (s.has_value()) {
      i++;
      tot += s.value();
    }
  }
  return (i > 0 ? tot / i : -1);
}
std::string Statistics::header() {
  std::stringstream s;
  s << "\t\tVarScore\tInvScore\tAnnScore\tWidth" << std::endl;
  line();
  return s.str();
}

std::string Statistics::line() {
  std::stringstream s;
  s << "----------------------------------------------------------------------"
    << std::endl;
  return s.str();
}
std::string Statistics::count() {
  std::stringstream s;
  s << "Count:\t\t" << variableCount() << "\t\t" << _model->constraints().size()
    << "\t\t" << annotationCount() << "\t\t-" << std::endl;
  return s.str();
}
std::string Statistics::row(int i) {
  std::stringstream s;
  s << "Scheme " << i << ":\t";
  if (variableScore()) {
    s << std::fixed << std::setprecision(2) << variableScore().value()
      << "%\t\t";
  } else {
    s << "\t-\t";
  }
  if (constraintScore()) {
    s << std::fixed << std::setprecision(2) << constraintScore().value()
      << "%\t\t";
  } else {
    s << "\t-\t";
  }
  if (matchingAnnotationsScore()) {
    s << std::fixed << std::setprecision(2)
      << matchingAnnotationsScore().value() << "%\t\t";
  } else {
    s << "\t-\t";
  }
  s << width(false);

  s << std::endl;
  return s.str();
}
