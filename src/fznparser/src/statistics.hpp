#pragma once

#include "model.hpp"

class Statistics {
 public:
  Statistics() = default;
  Statistics(Model* model, bool ignoreDynamicCycles) {
    _model = model;
    _ignoreDynamicCycles = ignoreDynamicCycles;
  };
  std::string header();
  std::string row();
  std::string line();
  std::string count();
  std::string info();
  void countDefinedVariables(bool labels);
  void variablesDefinedBy();
  void freeVariables();
  void enlDomVariables();
  void allStats(bool labels);
  void constraints(bool labels);
  int countImplicit();
  int countPotImplicit();
  int countSoft();
  int countBadSoft();
  int countOnlySoft();
  int variableCount();
  int noPotDefCount();
  int definedCount();
  int annotationCount();
  int enlargedDomains();
  int width(bool labels);
  void width_aux(std::vector<Node*>& result, std::vector<Node*> visited,
                 Node* node, int x, int& w);
  void matchingAnnotations(bool labels);

  double score();
  std::optional<double> matchingAnnotationsScore();
  std::optional<double> variableScore();
  std::optional<double> constraintScore();

  int influence(Variable* variable);
  void influenceAux(int& influence, std::set<Variable*>& visited,
                    Variable* variable);
  double averageInfluence();
  double maxInfluence();
  int dependency(Variable* variable);
  void dependencyAux(int& dependency, std::set<Variable*>& visited,
                     Variable* variable);
  double averageDependency();
  double maxDependency();
  std::vector<Variable*> outputVars();
  std::vector<Variable*> inputVars();
  int neighbourhoodSize();

  Model* _model;
  bool _ignoreDynamicCycles;
};
