#include "invariantstructure.hpp"

#include <sstream>

void InvariantStructure::run() {
  std::stringstream s;
  s << _stats.line();
  s << _stats.header();
  s << _stats.line();
  _schemes.scheme1();
  s << _stats.row(1);
  _schemes.scheme2();
  s << _stats.row(2);
  _schemes.scheme3();
  s << _stats.row(3);
  _schemes.scheme4();
  s << _stats.row(4);
  _schemes.scheme5();
  s << _stats.row(5);
  _schemes.scheme6();
  s << _stats.row(6);
  s << _stats.line();
  s << _stats.count();
  s << _stats.line();
  std::cout << s.str();
  _stats.allStats(true);
}
