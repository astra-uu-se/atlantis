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

  /** Destructor.
   */
  ~ModelNotOpenException() {}
};

class PropagationGraphHasCycles : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit PropagationGraphHasCycles(const std::string& msg)
      : std::runtime_error(msg) {}

  /** Destructor.
   */
  ~PropagationGraphHasCycles() {}
};