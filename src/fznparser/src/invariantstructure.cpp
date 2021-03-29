#include "invariantstructure.hpp"

#include <sstream>
InvariantStructure::InvariantStructure(Model m, std::string args)
    : _model(m), _stats(Statistics(&_model)), _schemes(Schemes(&_model)) {
  std::set<char> argset;
  for (auto l : args) {
    argset.insert(l);
  }
  if (argset.count('f')) {
    _fullStats = true;
  }
  if (argset.count('a')) {
    _allStats = true;
  }
}

void InvariantStructure::run() {
  std::stringstream s;
  s << _stats.line();
  s << _stats.header();
  s << _stats.line();
  _schemes.scheme1();
  s << _stats.row(1);
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme2();
  s << _stats.row(2);
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme3();
  s << _stats.row(3);
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme4();
  s << _stats.row(4);
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme5();
  s << _stats.row(5);
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme6();
  s << _stats.row(6);
  if (_allStats) _stats.allStats(_fullStats);
  s << _stats.line();
  s << _stats.count();
  s << _stats.line();
  std::cout << s.str();
}
