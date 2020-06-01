#pragma once
#include "core/factory.hpp"
#include "core/types.hpp"

class Engine {
 private:
  Factory m_factory;

 public:
  Engine(/* args */);
  ~Engine();

  /**
   * Register that target depends on dependency
   */
  void registerDependency(Id target, Id dependcy);
};