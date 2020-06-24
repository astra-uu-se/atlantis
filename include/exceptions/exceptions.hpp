#pragma once
#include <stdexcept>

class VariableAlreadyDefinedException : public std::runtime_error {

public:
  /** 
   * @param msg The error message
   */
  explicit 
  VariableAlreadyDefinedException(const std::string& msg) :
    std::runtime_error(msg) {}

  /** Destructor.
   */
  ~VariableAlreadyDefinedException() {}
};

class ModelNotOpenException : public std::runtime_error {

public:
  /** 
   * @param msg The error message
   */
  explicit 
  ModelNotOpenException(const std::string& msg) :
    std::runtime_error(msg) {}
};

class PropagationGraphHasCycles : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit PropagationGraphHasCycles(const std::string& msg)
      : std::runtime_error(msg) {}
};

class FailedToInitialise : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit FailedToInitialise()
      : std::runtime_error("Failed to initialise, possibly due to cycle in invariant graph.") {}
};