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
  _latex = argset.count('l') > 0;
  _noWidth = argset.count('w') > 0;
  _stats = Statistics(&_model, _ignoreDynamicCycles, _noWidth);
  _schemes = Schemes(&_model, _ignoreDynamicCycles);
}

void InvariantStructure::runLatex() {
  std::stringstream s;
  s << _stats.latexHeader();
  _schemes.random();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.annOnly();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.annImp();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.scheme1();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.scheme2();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.scheme5();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.scheme6();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.scheme7();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.scheme10();
  s << _schemes.name();
  s << _stats.latexRow();

  _schemes.scheme9();
  s << _schemes.name();
  s << _stats.latexRow();

  s << "\\hline" << std::endl;
  s << "\\hline" << std::endl;
  s << _stats.latexCount();
  std::cout << s.str();
}

void InvariantStructure::run() {
  if (_latex) {
    runLatex();
    return;
  }
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

  _schemes.annOnly();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);

  _schemes.annImp();
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

  _schemes.scheme10();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);

  _schemes.scheme9();
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
  _schemes.random();
  s << _schemes.name();
  s << _stats.row();
  if (_allStats) _stats.allStats(_fullStats);
  s << _stats.line();
  s << _stats.count();
  s << _stats.line();
  std::cout << s.str();
}
