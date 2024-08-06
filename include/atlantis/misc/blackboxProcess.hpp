#pragma once

#include <string>
#include <thread>
#include <vector>

namespace atlantis::blackbox {

/// Platform process session used by the executable blackbox backend.
///
/// A session owns exactly one child process and is owned by exactly one search
/// thread. The backend keeps one session per thread, so `exchange` never needs
/// to be serialised: the thread that created the session is the only one that
/// ever talks to it.
class BlackBoxProcessSession {
 protected:
  std::thread::id _owner;

  BlackBoxProcessSession() : _owner(std::this_thread::get_id()) {}

 public:
  virtual ~BlackBoxProcessSession() = default;

  BlackBoxProcessSession(const BlackBoxProcessSession&) = delete;
  BlackBoxProcessSession& operator=(const BlackBoxProcessSession&) = delete;

  [[nodiscard]] bool ownedByCurrentThread() const {
    return _owner == std::this_thread::get_id();
  }

  /// Send \a request to the child process and return its response line.
  virtual std::string exchange(const std::string& request) = 0;
};

/// Create the process implementation selected for the target platform.
///
/// Throws when the platform has no supported implementation.
BlackBoxProcessSession* createBlackBoxProcess(
    const std::string& program, const std::vector<std::string>& args);

}  // namespace atlantis::blackbox
