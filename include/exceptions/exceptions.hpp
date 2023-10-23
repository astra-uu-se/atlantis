#pragma once
#include <stdexcept>

namespace atlantis {

class VariableAlreadyDefinedException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit VariableAlreadyDefinedException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class EngineOpenException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit EngineOpenException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class EngineClosedException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit EngineClosedException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class EngineStateException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit EngineStateException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class PropagationGraphHasCycles : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit PropagationGraphHasCycles(const std::string& msg)
      : std::runtime_error(msg) {}
};

class BadEngineState : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit BadEngineState(const std::string& msg) : std::runtime_error(msg) {}
};

class FailedToInitialise : public std::runtime_error {
 public:
  explicit FailedToInitialise()
      : std::runtime_error(
            "Failed to initialise, possibly due to cycle in invariant graph.") {
  }
};

class TopologicalOrderError : public std::runtime_error {
 public:
  explicit TopologicalOrderError()
      : std::runtime_error(
            "Could not topologically order the invariant graph due to one or "
            "more dynamic cycles.") {}
};

// We do not extend std::runtime_error to keep runtime overhead at a minimum.
class DynamicCycleException : public std::exception {
 public:
  explicit DynamicCycleException() = default;
};

class OutOfOrderIndexRegistration : public std::exception {
 public:
  explicit OutOfOrderIndexRegistration() = default;
};

class VariableIsNotSearchVariable : public std::exception {
 public:
  explicit VariableIsNotSearchVariable() = default;
};

}