#pragma once
#include <stdexcept>

namespace atlantis {

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

class VarIsNotSearchVar : public std::exception {
 public:
  explicit VarIsNotSearchVar() = default;
};

class Exception : public std::exception {
 protected:
  /** Error message.
   */
  std::string _msg;

 public:
  /** Constructor (C strings).
   *  @param message C-style string error message.
   *                 The string contents are copied upon construction.
   *                 Hence, responsibility for deleting the char* lies
   *                 with the caller.
   */
  explicit Exception(const char* msg) : _msg(msg) {}

  /** Constructor (C++ STL strings).
   *  @param message The error message.
   */
  explicit Exception(const std::string& msg) : _msg(msg) {}

  /** Destructor.
   * Virtual to allow for subclassing.
   */
  virtual ~Exception() noexcept {}

  /** Returns a reference to the (constant) error description.
   *  @return the exception message.
   */

  virtual const std::string& msg() const noexcept { return _msg; }
};

class FznArgumentException : public Exception {
 public:
  explicit FznArgumentException(const char* msg) : Exception(msg) {}
  explicit FznArgumentException(const std::string& msg) : Exception(msg) {}
};

class InvariantGraphException : public Exception {
 public:
  explicit InvariantGraphException(const char* msg) : Exception(msg) {}
  explicit InvariantGraphException(const std::string& msg) : Exception(msg) {}
};

}  // namespace atlantis