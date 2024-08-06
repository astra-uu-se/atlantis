#include "atlantis/misc/blackboxProcess.hpp"

#ifdef _WIN32

#define NOMINMAX  // Ensure the words min/max remain available
#include <Windows.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace atlantis::blackbox {
namespace {

/// Upper bound on a single response, so a runaway child cannot exhaust memory.
constexpr size_t MAX_EXEC_RESPONSE_SIZE = 1024 * 1024;

std::string windowsError(const std::string& prefix, DWORD err) {
  return "BlackBoxExec: " + prefix + " (Windows error " + std::to_string(err) +
         ")";
}

std::wstring utf8ToWide(const std::string& s) {
  if (s.empty()) {
    return {};
  }
  const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(),
                                    static_cast<int>(s.size()), nullptr, 0);
  if (n == 0) {
    throw std::runtime_error(
        "BlackBox: invalid UTF-8 string in blackbox path or argument");
  }
  std::wstring w(static_cast<size_t>(n), L'\0');
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(),
                          static_cast<int>(s.size()), w.data(), n) == 0) {
    throw std::runtime_error(
        "BlackBox: invalid UTF-8 string in blackbox path or argument");
  }
  return w;
}

class WindowsHandle {
  HANDLE _handle;

 public:
  explicit WindowsHandle(HANDLE handle = nullptr) : _handle(handle) {}
  ~WindowsHandle() { reset(); }

  WindowsHandle(const WindowsHandle&) = delete;
  WindowsHandle& operator=(const WindowsHandle&) = delete;

  [[nodiscard]] HANDLE get() const { return _handle; }

  HANDLE* put() {
    reset();
    return &_handle;
  }

  HANDLE release() {
    HANDLE h = _handle;
    _handle = nullptr;
    return h;
  }

  [[nodiscard]] bool valid() const {
    return (_handle != nullptr) && (_handle != INVALID_HANDLE_VALUE);
  }

  void reset(HANDLE handle = nullptr) {
    if (valid()) {
      CloseHandle(_handle);
    }
    _handle = handle;
  }
};

class WindowsAttributeList {
  std::vector<char> _buffer;
  LPPROC_THREAD_ATTRIBUTE_LIST _list{nullptr};
  bool _initialized{false};

 public:
  WindowsAttributeList() = default;
  ~WindowsAttributeList() {
    if (_initialized) {
      DeleteProcThreadAttributeList(_list);
    }
  }

  WindowsAttributeList(const WindowsAttributeList&) = delete;
  WindowsAttributeList& operator=(const WindowsAttributeList&) = delete;

  void init() {
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    if (size == 0) {
      throw std::runtime_error(windowsError(
          "ProcThreadAttributeList size query failed", GetLastError()));
    }
    _buffer.resize(size);
    _list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(_buffer.data());
    if (InitializeProcThreadAttributeList(_list, 1, 0, &size) == 0) {
      throw std::runtime_error(windowsError(
          "InitializeProcThreadAttributeList failed", GetLastError()));
    }
    _initialized = true;
  }

  void setInheritedHandles(HANDLE* handles, DWORD count) {
    if (UpdateProcThreadAttribute(_list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                  handles, sizeof(HANDLE) * count, nullptr,
                                  nullptr) == 0) {
      throw std::runtime_error(windowsError(
          "PROC_THREAD_ATTRIBUTE_HANDLE_LIST failed", GetLastError()));
    }
  }

  [[nodiscard]] LPPROC_THREAD_ATTRIBUTE_LIST get() const { return _list; }
};

bool qualifiedPath(const std::wstring& program) {
  return (program.find_first_of(L"\\/") != std::wstring::npos) ||
         ((program.size() > 1) && (program[1] == L':'));
}

/// Quote an argument following the MSVC command-line parsing rules.
std::wstring quoteArgument(const std::wstring& arg) {
  std::wstring q(L"\"");
  unsigned int backslashes = 0;
  for (const wchar_t ch : arg) {
    if (ch == L'\\') {
      ++backslashes;
    } else if (ch == L'"') {
      q.append(backslashes * 2 + 1, L'\\');
      q += ch;
      backslashes = 0;
    } else {
      q.append(backslashes, L'\\');
      q += ch;
      backslashes = 0;
    }
  }
  q.append(backslashes * 2, L'\\');
  q += L'"';
  return q;
}

class WindowsProcessSession : public BlackBoxProcessSession {
  HANDLE _job{nullptr};
  HANDLE _process{nullptr};
  HANDLE _pipeSend{nullptr};
  HANDLE _pipeReceive{nullptr};

  static std::string lastError(const std::string& prefix) {
    return windowsError(prefix, GetLastError());
  }

  static void closeHandle(HANDLE& h) {
    if (h != nullptr) {
      CloseHandle(h);
      h = nullptr;
    }
  }

  void openWindows(const std::string& program,
                   const std::vector<std::string>& args);
  void closeWindows();

 public:
  WindowsProcessSession(const std::string& program,
                        const std::vector<std::string>& args) {
    openWindows(program, args);
  }

  ~WindowsProcessSession() override { closeWindows(); }

  std::string exchange(const std::string& request) override {
    size_t written = 0;
    while (written < request.size()) {
      DWORD count = 0;
      const auto remaining = static_cast<DWORD>(request.size() - written);
      const BOOL success = WriteFile(_pipeSend, request.data() + written,
                                     remaining, &count, nullptr);
      if ((success == 0) || (count == 0)) {
        throw std::runtime_error(
            lastError("writing blackbox process input failed"));
      }
      written += count;
    }

    char c[2] = {0, 0};
    std::ostringstream oss;
    size_t responseSize = 0;
    while (c[0] != '\n') {
      DWORD count = 0;
      const BOOL success =
          ReadFile(_pipeReceive, c, sizeof(c) - 1, &count, nullptr);
      if (success == 0) {
        if (GetLastError() == ERROR_BROKEN_PIPE) {
          throw std::runtime_error(
              "BlackBoxExec: blackbox process provided an incomplete response");
        }
        throw std::runtime_error(
            lastError("failed to read blackbox process output from pipe"));
      }
      if (count == 0) {
        throw std::runtime_error(
            "BlackBoxExec: blackbox process provided an incomplete response");
      }
      if (++responseSize > MAX_EXEC_RESPONSE_SIZE) {
        throw std::runtime_error(
            "BlackBoxExec: blackbox process response exceeds the size limit");
      }
      oss << c[0];
    }
    return oss.str();
  }
};

void WindowsProcessSession::openWindows(const std::string& program,
                                        const std::vector<std::string>& args) {
  // Build the command line before opening OS handles so allocation/conversion
  // failures cannot leak partially constructed process state.
  const std::wstring programW = utf8ToWide(program);
  std::wstring prog = quoteArgument(programW);
  for (const std::string& a : args) {
    prog += L" ";
    prog += quoteArgument(utf8ToWide(a));
  }
  std::vector<wchar_t> cmdline(prog.begin(), prog.end());
  cmdline.push_back(L'\0');

  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = nullptr;

  WindowsHandle childStdinRead;
  WindowsHandle childStdinWrite;
  WindowsHandle childStdoutRead;
  WindowsHandle childStdoutWrite;
  WindowsHandle childStderrWrite;

  if (CreatePipe(childStdoutRead.put(), childStdoutWrite.put(), &saAttr, 0) ==
      0) {
    throw std::runtime_error(lastError("stdout CreatePipe failed"));
  }
  if (SetHandleInformation(childStdoutRead.get(), HANDLE_FLAG_INHERIT, 0) == 0) {
    throw std::runtime_error(lastError("stdout SetHandleInformation failed"));
  }
  if (CreatePipe(childStdinRead.put(), childStdinWrite.put(), &saAttr, 0) == 0) {
    throw std::runtime_error(lastError("stdin CreatePipe failed"));
  }
  if (SetHandleInformation(childStdinWrite.get(), HANDLE_FLAG_INHERIT, 0) == 0) {
    throw std::runtime_error(lastError("stdin SetHandleInformation failed"));
  }

  HANDLE parentStderr = GetStdHandle(STD_ERROR_HANDLE);
  if ((parentStderr != nullptr) && (parentStderr != INVALID_HANDLE_VALUE)) {
    if (DuplicateHandle(GetCurrentProcess(), parentStderr, GetCurrentProcess(),
                        childStderrWrite.put(), 0, TRUE,
                        DUPLICATE_SAME_ACCESS) == 0) {
      throw std::runtime_error(lastError("stderr DuplicateHandle failed"));
    }
  } else {
    HANDLE nul =
        CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    &saAttr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nul == INVALID_HANDLE_VALUE) {
      throw std::runtime_error(lastError("stderr NUL CreateFile failed"));
    }
    childStderrWrite.reset(nul);
  }

  WindowsAttributeList attrList;
  attrList.init();

  PROCESS_INFORMATION piProcInfo;
  STARTUPINFOEXW siStartInfo;
  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFOEXW));
  siStartInfo.StartupInfo.cb = sizeof(STARTUPINFOEXW);
  siStartInfo.StartupInfo.hStdOutput = childStdoutWrite.get();
  siStartInfo.StartupInfo.hStdInput = childStdinRead.get();
  siStartInfo.StartupInfo.hStdError = childStderrWrite.get();
  siStartInfo.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

  HANDLE inheritHandles[3] = {childStdinRead.get(), childStdoutWrite.get(),
                              childStderrWrite.get()};
  attrList.setInheritedHandles(inheritHandles, 3);
  siStartInfo.lpAttributeList = attrList.get();

  // A job object with KILL_ON_JOB_CLOSE guarantees the child cannot outlive
  // the solver, even if the solver dies abruptly.
  WindowsHandle processJob(CreateJobObjectW(nullptr, nullptr));
  if (!processJob.valid()) {
    throw std::runtime_error(lastError("CreateJobObject failed"));
  }
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo;
  ZeroMemory(&jobInfo, sizeof(jobInfo));
  jobInfo.BasicLimitInformation.LimitFlags =
      JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (SetInformationJobObject(processJob.get(),
                              JobObjectExtendedLimitInformation, &jobInfo,
                              sizeof(jobInfo)) == 0) {
    throw std::runtime_error(lastError("SetInformationJobObject failed"));
  }

  const BOOL processStarted = CreateProcessW(
      qualifiedPath(programW) ? programW.c_str() : nullptr, cmdline.data(),
      nullptr,  // process security attributes
      nullptr,  // primary thread security attributes
      TRUE,     // handles from attribute list
      EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED,
      nullptr,  // use parent's environment
      nullptr,  // use parent's current directory
      &siStartInfo.StartupInfo, &piProcInfo);

  if (processStarted == 0) {
    throw std::runtime_error(
        windowsError("starting blackbox process failed for program `" +
                         program + "'",
                     GetLastError()));
  }

  WindowsHandle processHandle(piProcInfo.hProcess);
  WindowsHandle threadHandle(piProcInfo.hThread);

  if (AssignProcessToJobObject(processJob.get(), processHandle.get()) == 0) {
    const DWORD err = GetLastError();
    DWORD terminateErr = ERROR_SUCCESS;
    if (TerminateProcess(processHandle.get(), 1) == 0) {
      terminateErr = GetLastError();
    }
    const DWORD wait = WaitForSingleObject(processHandle.get(), 5000);
    std::string message =
        windowsError("unable to assign blackbox process to required job", err);
    if (terminateErr != ERROR_SUCCESS) {
      message +=
          "; " + windowsError("TerminateProcess cleanup failed", terminateErr);
    }
    if (wait == WAIT_FAILED) {
      message += "; " + lastError("process cleanup wait failed");
    } else if (wait == WAIT_TIMEOUT) {
      message += "; process cleanup timed out";
    }
    throw std::runtime_error(message);
  }

  if (ResumeThread(threadHandle.get()) == static_cast<DWORD>(-1)) {
    const DWORD err = GetLastError();
    DWORD terminateErr = ERROR_SUCCESS;
    if (TerminateJobObject(processJob.get(), 1) == 0) {
      terminateErr = GetLastError();
    }
    HANDLE assignedJob = processJob.release();
    DWORD closeErr = ERROR_SUCCESS;
    if (CloseHandle(assignedJob) == 0) {
      closeErr = GetLastError();
    }
    const DWORD wait = WaitForSingleObject(processHandle.get(), 5000);
    std::string message =
        windowsError("ResumeThread failed for blackbox process", err);
    if (terminateErr != ERROR_SUCCESS) {
      message += "; " + windowsError("TerminateJobObject cleanup failed",
                                     terminateErr);
    }
    if (closeErr != ERROR_SUCCESS) {
      message += "; " + windowsError("job cleanup close failed", closeErr);
    }
    if (wait == WAIT_FAILED) {
      message += "; " + lastError("process cleanup wait failed");
    } else if (wait == WAIT_TIMEOUT) {
      message += "; process cleanup timed out";
    }
    throw std::runtime_error(message);
  }

  _pipeSend = childStdinWrite.release();
  _pipeReceive = childStdoutRead.release();
  _process = processHandle.release();
  _job = processJob.release();
}

void WindowsProcessSession::closeWindows() {
  closeHandle(_pipeSend);
  closeHandle(_pipeReceive);
  if (_process != nullptr) {
    const DWORD wait = WaitForSingleObject(_process, 1000);
    if (wait == WAIT_TIMEOUT) {
      if (_job != nullptr) {
        TerminateJobObject(_job, 1);
      } else {
        TerminateProcess(_process, 1);
      }
      WaitForSingleObject(_process, 5000);
    }
    closeHandle(_process);
  }
  closeHandle(_job);
}

}  // namespace

BlackBoxProcessSession* createBlackBoxProcess(
    const std::string& program, const std::vector<std::string>& args) {
  return new WindowsProcessSession(program, args);
}

}  // namespace atlantis::blackbox

#endif
