#pragma once
#include <stdexcept>

namespace atlantis {

class VarException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit VarException(const std::string& msg) : std::runtime_error(msg) {}
};

class VarAlreadyDefinedException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit VarAlreadyDefinedException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class SolverOpenException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit SolverOpenException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class SolverClosedException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit SolverClosedException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class SolverStateException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit SolverStateException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class InvariantGraphException : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit InvariantGraphException(const std::string& msg)
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

class BadSolverState : public std::runtime_error {
 public:
  /**
   * @param msg The error message
   */
  explicit BadSolverState(const std::string& msg) : std::runtime_error(msg) {}
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

class FznArgumentException : public std::runtime_error {
 public:
  explicit FznArgumentException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class InfeasibleException : public std::runtime_error {
 public:
  explicit InfeasibleException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class DomainException : public std::runtime_error {
 public:
  explicit DomainException(const std::string& msg) : std::runtime_error(msg) {}
};

class InconsistencyException : public std::runtime_error {
 public:
  explicit InconsistencyException(const std::string& msg)
      : std::runtime_error(msg) {}
};

}  // namespace atlantis