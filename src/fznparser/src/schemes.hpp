#pragma once

#include "model.hpp"
#include "variable.hpp"

class Schemes {
 public:
  Schemes() = default;
  Schemes(Model* m, bool ignoreDynamicCycles) {
    _m = m;
    _ignoreDynamicCycles = ignoreDynamicCycles;
  }

  std::string name();
  void scheme1();
  void scheme2();
  void scheme3();
  void scheme4();
  void scheme5();
  void scheme6();
  void scheme7();
  void scheme8();
  void scheme9();
  void scheme10();
  void random();
  void annOnly();
  void annImp();
  void reset();
  void defineRandom();
  void defineAnnotated();
  void defineImplicit();
  void defineFrom(Variable* variable);
  void defineFromWithImplicit(Variable* variable);
  void defineFromObjective();
  void defineUnique();
  void defineRest();
  void defineByImplicit();
  std::vector<Node*> hasCycle();
  std::vector<Node*> checkDynamicCycle(std::vector<Node*> stack);
  bool hasCycleAux(std::set<Node*>& visited, std::vector<Node*>& stack,
                   Node* n);
  void removeCycles(bool ban);
  void removeCycle(std::vector<Node*> visited, bool ban);
  int cyclesRemoved() { return _cyclesRemoved; }
  void updateDomains();
  void defineLeastUsed();

 private:
  std::string _name;
  Model* _m;
  int _cyclesRemoved = 0;
  bool _ignoreDynamicCycles = false;
};
