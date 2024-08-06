#include "atlantis/misc/blackboxProcess.hpp"

#if defined(ATLANTIS_HAS_POSIX_BLACKBOX_EXEC) && !defined(_WIN32)

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <crt_externs.h>
#define ATLANTIS_ENVIRON (*_NSGetEnviron())
#else
extern char** environ;
#define ATLANTIS_ENVIRON environ
#endif

namespace atlantis::blackbox {
namespace {

/// Upper bound on a single response, so a runaway child cannot exhaust memory.
constexpr size_t MAX_EXEC_RESPONSE_SIZE = 1024 * 1024;

std::string lastError(const std::string& prefix) {
  return "BlackBoxExec: " + prefix + " (errno " + std::to_string(errno) + ")";
}

int setCloexec(int fd) {
  const int flags = fcntl(fd, F_GETFD);
  if (flags == -1) {
    return -1;
  }
  return fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

int dupCloexec(int fd, int minFd) {
  int nfd = -1;
#ifdef F_DUPFD_CLOEXEC
  nfd = fcntl(fd, F_DUPFD_CLOEXEC, minFd);
  if (nfd != -1) {
    return nfd;
  }
  if (errno != EINVAL) {
    return -1;
  }
#endif
  nfd = fcntl(fd, F_DUPFD, minFd);
  if (nfd == -1) {
    return -1;
  }
  if (setCloexec(nfd) != 0) {
    const int e = errno;
    ::close(nfd);
    errno = e;
    return -1;
  }
  return nfd;
}

/// Move \a fd above stderr so that the dup2 targets used while spawning cannot
/// collide with the descriptors we are about to install.
int moveFromStandardFd(int fd) {
  if (fd > STDERR_FILENO) {
    return fd;
  }
  const int nfd = dupCloexec(fd, STDERR_FILENO + 1);
  if (nfd == -1) {
    return -1;
  }
  ::close(fd);
  return nfd;
}

class FileDescriptor {
  int _fd;

 public:
  explicit FileDescriptor(int fd = -1) : _fd(fd) {}
  ~FileDescriptor() { reset(); }

  FileDescriptor(const FileDescriptor&) = delete;
  FileDescriptor& operator=(const FileDescriptor&) = delete;

  [[nodiscard]] int get() const { return _fd; }

  int release() {
    const int fd = _fd;
    _fd = -1;
    return fd;
  }

  void reset(int fd = -1) {
    if (_fd != -1) {
      ::close(_fd);
    }
    _fd = fd;
  }
};

int moveAwayFromStandardFd(FileDescriptor& fd) {
  const int old = fd.release();
  const int nfd = moveFromStandardFd(old);
  if (nfd == -1) {
    fd.reset(old);
  } else {
    fd.reset(nfd);
  }
  return nfd;
}

class SpawnFileActions {
  posix_spawn_file_actions_t _actions;
  bool _initialized{false};

 public:
  SpawnFileActions() = default;
  ~SpawnFileActions() {
    if (_initialized) {
      posix_spawn_file_actions_destroy(&_actions);
    }
  }

  SpawnFileActions(const SpawnFileActions&) = delete;
  SpawnFileActions& operator=(const SpawnFileActions&) = delete;

  int init() {
    const int err = posix_spawn_file_actions_init(&_actions);
    _initialized = err == 0;
    return err;
  }
  posix_spawn_file_actions_t* get() { return &_actions; }
};

class SpawnAttributes {
  posix_spawnattr_t _attr;
  bool _initialized{false};

 public:
  SpawnAttributes() = default;
  ~SpawnAttributes() {
    if (_initialized) {
      posix_spawnattr_destroy(&_attr);
    }
  }

  SpawnAttributes(const SpawnAttributes&) = delete;
  SpawnAttributes& operator=(const SpawnAttributes&) = delete;

  int init() {
    const int err = posix_spawnattr_init(&_attr);
    _initialized = err == 0;
    return err;
  }
  posix_spawnattr_t* get() { return &_attr; }
};

int createSocketpair(int sv[2]) {
#ifdef SOCK_CLOEXEC
  if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) == 0) {
    return 0;
  }
  if (errno != EINVAL) {
    return -1;
  }
#endif
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
    return -1;
  }
  if ((setCloexec(sv[0]) != 0) || (setCloexec(sv[1]) != 0)) {
    const int e = errno;
    ::close(sv[0]);
    ::close(sv[1]);
    errno = e;
    return -1;
  }
  return 0;
}

/// Write without ever raising SIGPIPE in the solver process: a blackbox that
/// dies must surface as an error, not as a fatal signal.
ssize_t sendNoSigpipe(int fd, const char* data, size_t size) {
#ifdef MSG_NOSIGNAL
  return send(fd, data, size, MSG_NOSIGNAL);
#elif defined(SO_NOSIGPIPE)
  return send(fd, data, size, 0);
#else
  sigset_t block;
  sigset_t old;
  sigset_t pending;
  sigemptyset(&block);
  sigaddset(&block, SIGPIPE);
  bool blocked = false;
  bool wasPending = false;
  if (pthread_sigmask(SIG_BLOCK, &block, &old) == 0) {
    blocked = true;
    if (sigpending(&pending) == 0) {
      wasPending = sigismember(&pending, SIGPIPE) == 1;
    }
  }
  const ssize_t n = send(fd, data, size, 0);
  if ((n == -1) && (errno == EPIPE) && !wasPending) {
    const struct timespec timeout = {0, 0};
    sigtimedwait(&block, nullptr, &timeout);
  }
  if (blocked) {
    pthread_sigmask(SIG_SETMASK, &old, nullptr);
  }
  return n;
#endif
}

class PosixProcessSession : public BlackBoxProcessSession {
  pid_t _child{-1};
  int _pipeSend{-1};
  FILE* _fileReceive{nullptr};

  static void sleepGracePeriod() {
    struct timespec remaining = {0, 10000000};
    while ((nanosleep(&remaining, &remaining) == -1) && (errno == EINTR)) {
    }
  }

  static bool childExited(pid_t pid) {
    siginfo_t info;
    do {
      info.si_pid = 0;
      if (waitid(P_PID, pid, &info, WEXITED | WNOHANG | WNOWAIT) == 0) {
        return info.si_pid != 0;
      }
    } while (errno == EINTR);
    return false;
  }

  static void signalGroup(pid_t pid, int sig) { kill(-pid, sig); }

  static void waitGroup(pid_t pid, int attempts) {
    for (int i = 0; i < attempts; ++i) {
      if ((kill(-pid, 0) == -1) && (errno == ESRCH)) {
        return;
      }
      if (childExited(pid)) {
        return;
      }
      sleepGracePeriod();
    }
  }

  static void terminateChild(pid_t pid) {
    if (pid <= 0) {
      return;
    }
    int status = 0;
    // Keep the child unreaped until the group has received both signals.
    signalGroup(pid, SIGTERM);
    waitGroup(pid, 100);
    signalGroup(pid, SIGKILL);
    do {
      if (waitpid(pid, &status, 0) != -1) {
        return;
      }
    } while (errno == EINTR);
  }

  /// Reaping the child requires default SIGCHLD handling; refuse to start
  /// rather than leak a zombie or block forever in waitpid.
  static void checkSigchld() {
    struct sigaction action{};
    if (sigaction(SIGCHLD, nullptr, &action) != 0) {
      throw std::runtime_error(lastError("SIGCHLD query failed"));
    }
    if ((action.sa_handler != SIG_DFL)
#ifdef SA_NOCLDWAIT
        || ((action.sa_flags & SA_NOCLDWAIT) != 0)
#endif
    ) {
      throw std::runtime_error(
          "BlackBoxExec: cannot start a blackbox process unless SIGCHLD uses "
          "SIG_DFL without SA_NOCLDWAIT");
    }
  }

  void openPosix(const std::string& program,
                 const std::vector<std::string>& args);
  void closePosix();

 public:
  PosixProcessSession(const std::string& program,
                      const std::vector<std::string>& args) {
    openPosix(program, args);
  }

  ~PosixProcessSession() override { closePosix(); }

  std::string exchange(const std::string& request) override {
    const char* p = request.c_str();
    size_t remaining = request.size();
    while (remaining > 0) {
      const ssize_t n = sendNoSigpipe(_pipeSend, p, remaining);
      if (n < 0) {
        if (errno == EINTR) {
          continue;
        }
        throw std::runtime_error(
            lastError("writing blackbox process input failed"));
      }
      if (n == 0) {
        throw std::runtime_error(
            "BlackBoxExec: writing blackbox process input wrote zero bytes");
      }
      p += n;
      remaining -= static_cast<size_t>(n);
    }

    std::string response;
    while (true) {
      errno = 0;
      const int ch = fgetc(_fileReceive);
      if (ch == EOF) {
        if (feof(_fileReceive) != 0) {
          throw std::runtime_error(
              "BlackBoxExec: blackbox process provided an incomplete response");
        }
        const int err = errno;
        if (err == EINTR) {
          clearerr(_fileReceive);
          continue;
        }
        errno = err;
        throw std::runtime_error(
            lastError("reading blackbox process output from pipe failed"));
      }
      response += static_cast<char>(ch);
      if (response.size() > MAX_EXEC_RESPONSE_SIZE) {
        throw std::runtime_error(
            "BlackBoxExec: blackbox process response exceeds the size limit");
      }
      if (ch == '\n') {
        break;
      }
    }
    return response;
  }
};

void PosixProcessSession::openPosix(const std::string& program,
                                    const std::vector<std::string>& args) {
  const int READ = 0;
  const int WRITE = 1;

  std::vector<char*> argv;
  argv.reserve(args.size() + 2);
  argv.push_back(const_cast<char*>(program.c_str()));
  for (const std::string& a : args) {
    argv.push_back(const_cast<char*>(a.c_str()));
  }
  argv.push_back(nullptr);

  checkSigchld();

  FileDescriptor childIn[2];
  FileDescriptor childOut[2];
  int fds[2];
  if (createSocketpair(fds) != 0) {
    throw std::runtime_error(lastError("stdin socket creation failed"));
  }
  childIn[READ].reset(fds[READ]);
  childIn[WRITE].reset(fds[WRITE]);
  if (createSocketpair(fds) != 0) {
    throw std::runtime_error(lastError("stdout socket creation failed"));
  }
  childOut[READ].reset(fds[READ]);
  childOut[WRITE].reset(fds[WRITE]);

  FileDescriptor* sessionFds[] = {&childIn[READ], &childIn[WRITE],
                                  &childOut[READ], &childOut[WRITE]};
  for (FileDescriptor* fd : sessionFds) {
    if (moveAwayFromStandardFd(*fd) == -1) {
      throw std::runtime_error(
          lastError("moving session descriptors away from stdio failed"));
    }
  }

  SpawnFileActions actions;
  int err = actions.init();
  if (err != 0) {
    errno = err;
    throw std::runtime_error(lastError("spawn file action init failed"));
  }

  SpawnAttributes attr;
  err = attr.init();
  if (err != 0) {
    errno = err;
    throw std::runtime_error(lastError("spawn attribute init failed"));
  }

  // Put the child in its own process group so that teardown can signal the
  // whole group without touching the solver process.
  err = posix_spawnattr_setpgroup(attr.get(), 0);
  if (err == 0) {
    short flags = POSIX_SPAWN_SETPGROUP;
#if defined(ATLANTIS_HAS_POSIX_SPAWN_CLOEXEC_DEFAULT) && \
    defined(ATLANTIS_HAS_POSIX_SPAWN_ADDINHERIT_NP)
    flags |= POSIX_SPAWN_CLOEXEC_DEFAULT;
#endif
    err = posix_spawnattr_setflags(attr.get(), flags);
  }
  if (err == 0) {
    err = posix_spawn_file_actions_adddup2(actions.get(), childIn[READ].get(),
                                           STDIN_FILENO);
  }
  if (err == 0) {
    err = posix_spawn_file_actions_adddup2(actions.get(), childOut[WRITE].get(),
                                           STDOUT_FILENO);
  }
#if defined(ATLANTIS_HAS_POSIX_SPAWN_CLOEXEC_DEFAULT) && \
    defined(ATLANTIS_HAS_POSIX_SPAWN_ADDINHERIT_NP)
  if (err == 0) {
    err = posix_spawn_file_actions_addinherit_np(actions.get(), STDERR_FILENO);
  }
#elif defined(ATLANTIS_HAS_POSIX_SPAWN_ADDCLOSEFROM_NP)
  if (err == 0) {
    err =
        posix_spawn_file_actions_addclosefrom_np(actions.get(),
                                                 STDERR_FILENO + 1);
  }
#endif
  if (err == 0) {
    err = posix_spawnp(&_child, program.c_str(), actions.get(), attr.get(),
                       argv.data(), ATLANTIS_ENVIRON);
  }
  if (err != 0) {
    _child = -1;
    errno = err;
    throw std::runtime_error(
        lastError("starting blackbox process `" + program + "' failed"));
  }

  childIn[READ].reset();
  childOut[WRITE].reset();

#ifdef SO_NOSIGPIPE
  const int nosigpipe = 1;
  if (setsockopt(childIn[WRITE].get(), SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe,
                 sizeof(nosigpipe)) != 0) {
    const int e = errno;
    terminateChild(_child);
    _child = -1;
    errno = e;
    throw std::runtime_error(lastError("SO_NOSIGPIPE setup failed"));
  }
#endif

  FILE* receive = fdopen(childOut[READ].get(), "r");
  if (receive == nullptr) {
    const int e = errno;
    terminateChild(_child);
    _child = -1;
    errno = e;
    throw std::runtime_error(lastError("fdopen failed"));
  }
  _fileReceive = receive;
  childOut[READ].release();
  _pipeSend = childIn[WRITE].release();
}

void PosixProcessSession::closePosix() {
  if (_pipeSend != -1) {
    ::close(_pipeSend);
    _pipeSend = -1;
  }
  if (_fileReceive != nullptr) {
    fclose(_fileReceive);
    _fileReceive = nullptr;
  }
  if (_child > 0) {
    terminateChild(_child);
    _child = -1;
  }
}

}  // namespace

BlackBoxProcessSession* createBlackBoxProcess(
    const std::string& program, const std::vector<std::string>& args) {
  return new PosixProcessSession(program, args);
}

}  // namespace atlantis::blackbox

#endif
