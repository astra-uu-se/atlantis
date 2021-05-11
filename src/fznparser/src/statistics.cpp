#include "statistics.hpp"

#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

int Statistics::variableCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefinable()) n++;
  }
  return n;
}
int Statistics::noPotDefCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefinable() && variable->noPotDef()) n++;
  }
  return n;
}

int Statistics::definedCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefined() && variable->isDefinable()) {
      n++;
    }
  }
  return n;
}
void Statistics::countDefinedVariables(bool labels) {
  std::cout << "===========================" << std::endl;
  std::cout << "=========VARIABLES=========" << std::endl;
  if (labels) {
    // variablesDefinedBy();
    // std::cout << "===========================" << std::endl;
    std::cout << "=========BAD=========" << std::endl;
    freeVariables();
    std::cout << "=========ENLARGED=========" << std::endl;
    enlDomVariables();
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
        if (variable->hasEnlargedDomain()) {
          std::cout << "(>)";
        }
      }
      std::cout << "\t[" << variable->definedBy()->getName() << "]"
                << std::endl;
    }
  }
}
void Statistics::freeVariables() {
  for (auto variable : _model->varMap().variables()) {
    if (!variable->isDefined() && variable->isDefinable() &&
        !variable->noPotDef()) {
      std::cout << "Var: " << variable->getName();
      std::cout << "\tDS: " << variable->domainSize();
      std::cout << "\tPot. Def:" << variable->orgPotDefSize();
      std::cout << std::endl;
    }
  }
}
void Statistics::enlDomVariables() {
  for (auto variable : _model->varMap().variables()) {
    if (variable->hasEnlargedDomain()) {
      std::cout << "Var: " << variable->getName();
      std::cout << "\tDS: " << variable->domainSize();
      std::cout << "\tORG: " << variable->_domain->size();
      std::cout << "\tPot. Def: " << variable->orgPotDefSize();
      std::cout << "\tDefined by: " << variable->definedBy()->getName();
      std::cout << std::endl;
    }
  }
}
int Statistics::countPotImplicit() {
  int imp = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isPotImplicit()) imp++;
  }
  return imp;
}
int Statistics::countImplicit() {
  int imp = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isImplicit()) imp++;
  }
  return imp;
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
int Statistics::countOnlySoft() {
  int soft = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isPotImplicit()) {
      continue;
    }
    if (constraint->isFunctional()) {
      bool functional = false;
      for (auto var : constraint->variables()) {
        if (var->hasPotentialDefiner(constraint)) functional = true;
      }
      if (functional) continue;
    }
    soft++;
  }
  return soft;
}

int Statistics::countBadSoft() {
  int soft = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isImplicit() || constraint->isInvariant()) {
      continue;
    }
    if (constraint->isPotImplicit()) {
      soft++;
    } else if (constraint->isFunctional()) {
      bool functional = false;
      for (auto var : constraint->variables()) {
        if (var->hasPotentialDefiner(constraint)) functional = true;
      }
      if (functional) soft++;
    }
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
      if (constraint->isFunctional() && !constraint->isInvariant()) {
        bool functional = false;
        for (auto var : constraint->variables()) {
          if (var->hasPotentialDefiner(constraint)) functional = true;
        }
        if (functional) {
          std::cout << constraint->getName() << std::endl;
        }
      }
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
  std::cout << line() << std::endl;
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
        // if (labels) {
        //   std::cout << constraint->getName() << " fulfills define
        //   annotation."
        //             << std::endl;
        // }
      } else {
        if (labels) {
          std::cout << constraint->getName() << " should define "
                    << constraint->annotationTarget().value()->getName()
                    << " but doesn't." << std::endl;
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
  vc -= noPotDefCount();
  if (vc > 0) {
    return 100 * definedCount() / (double)vc;
  }
  return {};
}
std::optional<double> Statistics::constraintScore() {
  if (_model->constraints().size() > 0) {
    return 100 *
           (_model->constraints().size() - countOnlySoft() - countBadSoft()) /
           ((double)_model->constraints().size() - countOnlySoft());
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
std::string Statistics::info() {
  std::stringstream s;
  s << line();
  s << "Information" << std::endl;
  s << line();
  s << "VarSco:\t\t";
  s << "Variable Score is the percentage of variables that are defined by an"
       "invariant or an implicit constraints."
    << std::endl;
  s << "InvSco:\t\t";
  s << "Invariant Score is the percentage of constraints that are either "
       "invariants or implicit constraints."
    << std::endl;
  s << "AnnSco:\t\t";
  s << "Annotation Score is the percentage of constraints with a defines_var "
       "annotation"
       "that define the targeted variable."
    << std::endl;
  s << "Height:\t\t";
  s << "Height is the length of the longest path in the graph."
       "(Not available when dynamic cycles are allowed.)"
    << std::endl;
  s << "In #:\t\t";
  s << "The amount of (definable) variables that are not defined." << std::endl;
  s << "Out #:\t\t";
  s << "The amount of (definable) variables that are defined and that are "
       "not "
       "input to another constraint."
    << std::endl;
  s << "Influence:\t";
  s << "Influence (average and max) is the percentage of variables that are "
       "children to an input variable."
    << std::endl;
  s << "Dependency:\t";
  s << "Dependency (average and max) is the percentage of variables that are "
       "ancestors to an output variable."
    << std::endl;
  s << "NbSum:\t\t";
  s << "The sum of the domains of the input variables." << std::endl;

  s << line();
  return s.str();
}
std::string Statistics::header() {
  std::stringstream s;
  s << "Scheme\t\tVarSco\tInvSco\tAnnSco\tImp\tHeight\tIn #\tOut "
       "#\tNbSum\tEnlDoms"
    << std::endl;
  line();
  return s.str();
}

std::string Statistics::line() {
  std::stringstream s;
  s << "---------------------------------------------------------------------"
       "--"
       "--------------------------------"
    << std::endl;
  return s.str();
}
std::string Statistics::count() {
  std::stringstream s;
  s << "Variables: " << variableCount() << std::endl
    << "Constraints: " << _model->constraints().size() << std::endl
    << "Annotations: " << annotationCount() << std::endl;
  return s.str();
}
std::string Statistics::row() {
  std::stringstream s;
  s << "\t";
  if (variableScore()) {
    s << std::fixed << std::setprecision(2) << variableScore().value() << "\t";
  } else {
    s << "-\t";
  }
  if (constraintScore()) {
    s << std::fixed << std::setprecision(2) << constraintScore().value()
      << "\t";
  } else {
    s << "-\t";
  }
  if (matchingAnnotationsScore()) {
    s << std::fixed << std::setprecision(2)
      << matchingAnnotationsScore().value() << "\t";
  } else {
    s << "-\t";
  }
  s << countImplicit() << "/" << countPotImplicit() << "\t";
  if (!_ignoreDynamicCycles) {
    s << 0;  // width(false);
  } else {
    s << "-";
  }
  s << "\t" << std::fixed << std::setprecision(2) << inputVars().size();
  // s << "\t" << std::fixed << std::setprecision(2) << averageInfluence();
  // s << "\t" << std::fixed << std::setprecision(2) << maxInfluence();
  s << "\t" << std::fixed << std::setprecision(2) << outputVars().size();
  // s << "\t" << std::fixed << std::setprecision(2) << averageDependency();
  // s << "\t" << std::fixed << std::setprecision(2) << maxDependency();
  s << "\t" << std::fixed << std::setprecision(2) << neighbourhoodSize();
  s << "\t" << enlargedDomains();

  s << std::endl;
  return s.str();
}

int Statistics::influence(Variable* variable) {
  int influence = 0;
  std::set<Variable*> visited;
  influenceAux(influence, visited, variable);
  return influence;
}
void Statistics::influenceAux(int& influence, std::set<Variable*>& visited,
                              Variable* variable) {
  if (visited.count(variable) || !variable->isDefinable()) return;
  influence++;
  visited.insert(variable);
  for (auto constraint : variable->getNextConstraint()) {
    for (auto var : constraint->getNextVariable()) {
      influenceAux(influence, visited, var);
    }
  }
}
double Statistics::averageInfluence() {
  int totalInfluence = 0;
  for (auto var : _model->varMap().variables()) {
    if (!var->isDefined() || var->definedBy()->isImplicit()) {
      totalInfluence += influence(var);
    }
  }
  return 100 * (double)totalInfluence /
         ((variableCount() - definedCount()) * (variableCount()));
}

double Statistics::maxInfluence() {
  int maxInfluence = 0;
  for (auto var : _model->varMap().variables()) {
    if (!var->isDefined()) {
      int newInfluence = influence(var);
      if (newInfluence > maxInfluence) {
        maxInfluence = newInfluence;
      }
    }
  }
  return 100 * (double)maxInfluence / (variableCount());
}
std::vector<Variable*> Statistics::inputVars() {
  std::vector<Variable*> inputVars;
  for (auto var : _model->varMap().variables()) {
    if (!var->isDefined() && var->isDefinable()) {
      inputVars.push_back(var);
    }
  }
  return inputVars;
}
std::vector<Variable*> Statistics::outputVars() {
  std::vector<Variable*> outputVars;
  for (auto var : _model->varMap().variables()) {
    if (var->isDefined() && var->getNext().empty()) {
      outputVars.push_back(var);
    }
  }
  return outputVars;
}
int Statistics::dependency(Variable* variable) {
  int dependency = 0;
  std::set<Variable*> visited;
  dependencyAux(dependency, visited, variable);
  return dependency;
}
void Statistics::dependencyAux(int& dependency, std::set<Variable*>& visited,
                               Variable* variable) {
  if (visited.count(variable) || !variable->isDefinable()) return;
  dependency++;
  visited.insert(variable);

  if (variable->isDefined()) {
    for (auto var : variable->definedBy()->dependencies()) {
      dependencyAux(dependency, visited, var);
    }
  }
}
double Statistics::averageDependency() {
  int totalDependency = 0;
  for (auto var : outputVars()) {
    totalDependency += dependency(var);
  }
  return 100 * (double)totalDependency /
         (outputVars().size() * (variableCount()));
}

double Statistics::maxDependency() {
  int maxDependency = 0;
  for (auto var : outputVars()) {
    int newDependency = dependency(var);
    if (newDependency > maxDependency) {
      maxDependency = newDependency;
    }
  }
  return 100 * (double)maxDependency / (variableCount());
}
int Statistics::neighbourhoodSize() {
  int nbhd = 0;
  for (auto var : inputVars()) {
    nbhd += var->domainSize();
  }
  return nbhd;
}

int Statistics::enlargedDomains() {
  int count = 0;
  for (auto var : _model->varMap().variables()) {
    if (var->hasEnlargedDomain()) count++;
  }
  return count;
}
