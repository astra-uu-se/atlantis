#include "invariantstructure.hpp"

#include <sstream>
InvariantStructure::InvariantStructure(Model m, std::string args) : _model(m) {
  std::set<char> argset;
  for (auto l : args) {
    argset.insert(l);
  }
  _fullStats = argset.count('f') > 0;
  _allStats = argset.count('a') > 0;
  _ignoreDynamicCycles = argset.count('c') > 0;
  _info = argset.count('i') > 0;
  _small = argset.count('s') > 0;
  _noStats = argset.count('n') > 0;
  _stats = Statistics(&_model, _ignoreDynamicCycles);
  _schemes = Schemes(&_model, _ignoreDynamicCycles);
}

void InvariantStructure::run() {
  if (_noStats) {
    _schemes.scheme1();
    _stats.allStats(false);
    return;
  }
  if (_small) {
    runSmall();
    return;
  }
  std::stringstream s;
  if (_info) s << _stats.info();
  s << _stats.line();
  s << _stats.header();
  s << _stats.line();
  _schemes.random();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme1();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme2();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme3();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme4();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme5();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme6();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme7();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme8();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme9();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  _schemes.scheme10();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  s << _stats.line();
  s << _stats.count();
  s << _stats.line();
  std::cout << s.str();
}

void InvariantStructure::runSmall() {
  std::stringstream s;
  if (_info) s << _stats.info();
  s << _stats.line();
  s << _stats.header();
  s << _stats.line();
  _schemes.scheme7();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  s << _stats.line();
  s << _stats.count();
  s << _stats.line();
  std::cout << s.str();
}
