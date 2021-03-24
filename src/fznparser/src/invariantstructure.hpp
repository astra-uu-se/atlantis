#include "model.hpp"
#include "schemes.hpp"
#include "statistics.hpp"

class InvariantStructure {
 public:
  InvariantStructure(Model m)
      : _model(m), _stats(Statistics(&_model)), _schemes(Schemes(&_model)) {}
  void run();

 private:
  Model _model;
  Statistics _stats;
  Schemes _schemes;
};
