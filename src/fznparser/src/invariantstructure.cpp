#include "invariantstructure.hpp"

void InvariantStructure::run() {
  _stats.header();
  _schemes.scheme1();
  _stats.row(1);
  _schemes.scheme2();
  _stats.row(2);
  _schemes.scheme3();
  _stats.row(3);
  _schemes.scheme4();
  _stats.row(4);
  _schemes.scheme5();
  _stats.row(5);
  _schemes.scheme6();
  _stats.row(6);
  _stats.line();
}
