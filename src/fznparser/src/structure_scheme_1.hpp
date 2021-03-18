#ifndef __STRUCTURE_SCHEME_1_HPP_INCLUDED__
#define __STRUCTURE_SCHEME_1_HPP_INCLUDED__
#include "model.hpp"
#include "variable.hpp"

class StructureScheme1 {
 public:
  StructureScheme1(Model* m) { _m = m; }

  void scheme1();
  void scheme2();
  void scheme3();
  void scheme4();
  void scheme5();
  void scheme6();
  void clear();
  void defineAnnotated();
  void defineImplicit();
  void defineFrom(Variable* variable);
  void defineFromWithImplicit(Variable* variable);
  void defineFromObjective();
  void defineUnique();
  void defineRest();
  void defineByImplicit();
  std::vector<Node*> hasCycle();
  bool hasCycleAux(std::set<Node*>& visited, std::vector<Node*>& stack,
                   Node* n);
  void removeCycles(bool ban);
  void removeCycle(std::vector<Node*> visited, bool ban);
  int cyclesRemoved() { return _cyclesRemoved; }
  void updateDomains();
  void defineLeastUsed();

 private:
  Model* _m;
  int _cyclesRemoved = 0;
};

#endif
