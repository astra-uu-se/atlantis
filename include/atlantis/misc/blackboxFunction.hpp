#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "atlantis/misc/blackboxProcess.hpp"
#include "atlantis/types.hpp"
#include "fznparser/annotation.hpp"

namespace atlantis::blackbox {

#ifdef _WIN32
#define ATLANTIS_BLACKBOX_CALL __stdcall
#else
#define ATLANTIS_BLACKBOX_CALL
#endif

/// Abstract class implemented by the different methods of running a blackbox
/// function.
///
/// A single instance is shared by every search thread, so implementations must
/// be safe to call concurrently. They achieve this by handing each thread its
/// own private resource (a cloned library instance or a dedicated child
/// process) rather than by serialising the calls themselves.
class BlackBoxFn {
 public:
  /// Create the backend described by a `blackbox_dll` / `blackbox_exec`
  /// annotation.
  static std::shared_ptr<BlackBoxFn> fromAnnotation(
      const fznparser::Annotation&);

  virtual ~BlackBoxFn() = default;

  BlackBoxFn(const BlackBoxFn&) = delete;
  BlackBoxFn& operator=(const BlackBoxFn&) = delete;

  /// Evaluate the blackbox. The output vectors are sized by the caller and are
  /// only written to by the blackbox.
  virtual void run(const std::vector<Int>& intIn,
                   const std::vector<double>& floatIn, std::vector<Int>& intOut,
                   std::vector<double>& floatOut) = 0;

 protected:
  BlackBoxFn() = default;
};

/// A blackbox implemented by a dynamically loaded library.
///
/// The library must export `fzn_blackbox`. It may additionally export
/// `fzn_init` to build a state object from the annotation arguments; a library
/// that exports `fzn_init` must also export `fzn_clone`, which is used to give
/// every search thread its own state. `fzn_free` is optional.
class BlackBoxDLL : public BlackBoxFn {
 public:
  BlackBoxDLL(const std::string& name, const std::vector<std::string>& args);
  ~BlackBoxDLL() override;

  void run(const std::vector<Int>& intIn, const std::vector<double>& floatIn,
           std::vector<Int>& intOut, std::vector<double>& floatOut) override;

 private:
  /// A library state object together with the thread that owns it.
  struct Instance {
    std::thread::id owner;
    void* value;
  };

  void* _library{nullptr};
  void*(ATLANTIS_BLACKBOX_CALL* _fznInit)(const char**, size_t){nullptr};
  void*(ATLANTIS_BLACKBOX_CALL* _fznClone)(void*){nullptr};
  void(ATLANTIS_BLACKBOX_CALL* _fznBlackbox)(void*, const int64_t*, size_t,
                                             const double*, size_t, int64_t*,
                                             size_t, double*, size_t){nullptr};
  void(ATLANTIS_BLACKBOX_CALL* _fznFree)(void*){nullptr};

  void* _rootInstance{nullptr};

  /// Guards `_instances` only: the blackbox call itself runs unlocked, since
  /// each thread has exclusive use of the instance it is handed.
  std::mutex _mutex;
  std::vector<Instance> _instances;

  /// Return the calling thread's instance, cloning it on first use.
  void* instance();
};

/// A blackbox implemented by a separate process, exchanging one request/response
/// line per evaluation over its stdin/stdout.
class BlackBoxExec : public BlackBoxFn {
 public:
  BlackBoxExec(std::string program, std::vector<std::string> args);
  ~BlackBoxExec() override;

  void run(const std::vector<Int>& intIn, const std::vector<double>& floatIn,
           std::vector<Int>& intOut, std::vector<double>& floatOut) override;

 private:
  std::string _program;
  std::vector<std::string> _args;

  /// Guards `_sessions` only; `exchange` runs unlocked on the thread's own
  /// session.
  std::mutex _mutex;
  std::vector<BlackBoxProcessSession*> _sessions;

  /// Return the calling thread's session, spawning the process on first use.
  BlackBoxProcessSession& session();
};

/// Encode a blackbox request: comma-separated integers, a semicolon, then
/// comma-separated floats, terminated by a newline (e.g. "5,-7;2.5,1.125\n").
std::string encodeBlackBoxRequest(const std::vector<Int>& intIn,
                                  const std::vector<double>& floatIn);

/// Parse a blackbox response into the (caller-sized) output vectors.
void decodeBlackBoxResponse(const std::string& response,
                            std::vector<Int>& intOut,
                            std::vector<double>& floatOut);

}  // namespace atlantis::blackbox
