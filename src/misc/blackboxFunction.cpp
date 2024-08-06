#include "atlantis/misc/blackboxFunction.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifdef _WIN32
#define NOMINMAX  // Ensure the words min/max remain available
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace atlantis::blackbox {

namespace {

/// fznparser hands string annotation arguments back with their quotes still
/// attached; strip them, but only when they really are there.
std::string stripQuotes(const std::string& s) {
  if ((s.size() >= 2) && (s.front() == '"') && (s.back() == '"')) {
    return s.substr(1, s.size() - 2);
  }
  return s;
}

/// A blackbox may only return finite floats inside the representable range.
/// The byte copy through `volatile` keeps the bit test from being folded away
/// by optimisers that assume NaN cannot occur.
void checkFloat(double v, size_t i) {
  static_assert(sizeof(double) == sizeof(std::uint64_t) &&
                    std::numeric_limits<double>::is_iec559,
                "blackbox floats must use IEEE-754 binary64");
  std::uint64_t bits = 0;
  const volatile unsigned char* raw =
      reinterpret_cast<const volatile unsigned char*>(&v);
  auto* target = reinterpret_cast<unsigned char*>(&bits);
  for (size_t j = 0; j < sizeof(bits); ++j) {
    target[j] = raw[j];
  }
  if ((bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000)) {
    throw std::runtime_error("BlackBox: blackbox output float " +
                             std::to_string(i) + " is not a finite value");
  }
}

#ifdef _WIN32

void* openLibrary(const std::string& name, std::string& loadError) {
  // Try the name as given, then the usual decorations.
  std::vector<std::string> candidates{name};
  if (name.size() < 4 ||
      _stricmp(name.c_str() + name.size() - 4, ".dll") != 0) {
    candidates.push_back(name + ".dll");
  }
  const size_t separator = name.find_last_of("\\/");
  const std::string directory =
      separator == std::string::npos ? "" : name.substr(0, separator + 1);
  const std::string basename =
      separator == std::string::npos ? name : name.substr(separator + 1);
  if (basename.compare(0, 3, "lib") != 0) {
    std::string prefixed = directory + "lib" + basename;
    if (prefixed.size() < 4 ||
        _stricmp(prefixed.c_str() + prefixed.size() - 4, ".dll") != 0) {
      prefixed += ".dll";
    }
    candidates.push_back(prefixed);
  }

  DWORD err = ERROR_FILE_NOT_FOUND;
  std::string failed = name;
  for (const std::string& candidate : candidates) {
    HMODULE handle = LoadLibraryA(candidate.c_str());
    if (handle != nullptr) {
      return reinterpret_cast<void*>(handle);
    }
    failed = candidate;
    err = GetLastError();
  }
  loadError = "unable to locate library `" + name + "' (LoadLibrary failed for `" +
              failed + "', Windows error " + std::to_string(err) + ")";
  return nullptr;
}

void closeLibrary(void* library) {
  if (library != nullptr) {
    FreeLibrary(static_cast<HMODULE>(library));
  }
}

/// Look up \a name, falling back to the 32-bit x86 stdcall decorations.
template <class T>
T librarySymbol(void* library, const char* name, unsigned int stdcallBytes) {
  FARPROC symbol = GetProcAddress(static_cast<HMODULE>(library), name);
#if defined(_M_IX86) || defined(__i386__)
  if (symbol == nullptr) {
    const std::string decorated =
        std::string("_") + name + "@" + std::to_string(stdcallBytes);
    symbol = GetProcAddress(static_cast<HMODULE>(library), decorated.c_str());
  }
  if (symbol == nullptr) {
    const std::string decorated =
        std::string(name) + "@" + std::to_string(stdcallBytes);
    symbol = GetProcAddress(static_cast<HMODULE>(library), decorated.c_str());
  }
#else
  (void)stdcallBytes;
#endif
  return reinterpret_cast<T>(symbol);
}

#else

void* openLibrary(const std::string& name, std::string& loadError) {
  void* loaded = dlopen(name.c_str(), RTLD_LAZY);
  if (loaded == nullptr) {
    const char* err = dlerror();
    loadError = err == nullptr ? "unable to open library" : err;
    loaded = dlopen((name + ".so").c_str(), RTLD_NOW);
  }
  if (loaded == nullptr) {
    loaded = dlopen(("lib" + name + ".so").c_str(), RTLD_NOW);
  }
#ifdef __APPLE__
  if (loaded == nullptr) {
    loaded = dlopen((name + ".dylib").c_str(), RTLD_NOW);
  }
  if (loaded == nullptr) {
    loaded = dlopen(("lib" + name + ".dylib").c_str(), RTLD_NOW);
  }
#endif
  return loaded;
}

void closeLibrary(void* library) {
  if (library != nullptr) {
    dlclose(library);
  }
}

template <class T>
T librarySymbol(void* library, const char* name, unsigned int) {
  T symbol = nullptr;
  // Avoids the object->function pointer cast warning.
  *reinterpret_cast<void**>(&symbol) = dlsym(library, name);
  return symbol;
}

#endif

}  // namespace

std::shared_ptr<BlackBoxFn> BlackBoxFn::fromAnnotation(
    const fznparser::Annotation& ann) {
  if (ann.expressions().empty() || ann.expressions().front().empty()) {
    throw std::runtime_error("BlackBox: annotation " + ann.identifier() +
                             " is missing its target argument");
  }
  if (!std::holds_alternative<std::string>(ann.expressions().front().front())) {
    throw std::runtime_error("BlackBox: annotation " + ann.identifier() +
                             " expects a string as its first argument");
  }
  const std::string target =
      stripQuotes(std::get<std::string>(ann.expressions().front().front()));

  // The second argument, when present, is the argument array passed on to the
  // blackbox (`blackbox_dll("lib.so", ["-v"])`).
  std::vector<std::string> args;
  if (ann.expressions().size() > 1) {
    for (const auto& expr : ann.expressions()[1]) {
      if (!std::holds_alternative<std::string>(expr)) {
        throw std::runtime_error(
            "BlackBox: annotation " + ann.identifier() +
            " expects an array of strings as its second argument");
      }
      args.push_back(stripQuotes(std::get<std::string>(expr)));
    }
  }

  if (ann.identifier() == "blackbox_dll") {
    return std::make_shared<BlackBoxDLL>(target, args);
  }
  if (ann.identifier() == "blackbox_exec") {
    return std::make_shared<BlackBoxExec>(target, std::move(args));
  }
  throw std::runtime_error("BlackBox: unknown blackbox annotation `" +
                           ann.identifier() + "'");
}

BlackBoxDLL::BlackBoxDLL(const std::string& name,
                         const std::vector<std::string>& args) {
  std::string loadError;
  void* loaded = openLibrary(name, loadError);
  if (loaded == nullptr) {
    throw std::runtime_error("BlackBox: unable to open dynamic library: " +
                             loadError);
  }

  bool rootInitialized = false;
  try {
    // The stdcall byte counts are the 32-bit x86 argument stack sizes.
    _fznBlackbox = librarySymbol<decltype(_fznBlackbox)>(
        loaded, "fzn_blackbox", 36);
    if (_fznBlackbox == nullptr) {
      throw std::runtime_error(
          "BlackBox: unable to find symbol `fzn_blackbox' in dynamic library `" +
          name + "'");
    }
    _fznInit = librarySymbol<decltype(_fznInit)>(loaded, "fzn_init", 8);
    _fznClone = librarySymbol<decltype(_fznClone)>(loaded, "fzn_clone", 4);
    _fznFree = librarySymbol<decltype(_fznFree)>(loaded, "fzn_free", 4);

    if ((_fznInit != nullptr) && (_fznClone == nullptr)) {
      throw std::runtime_error(
          "BlackBox: dynamic library `" + name +
          "' exports `fzn_init' but not `fzn_clone'");
    }

    if (_fznInit != nullptr) {
      std::vector<const char*> argv;
      argv.reserve(args.size());
      for (const std::string& arg : args) {
        argv.push_back(arg.c_str());
      }
      _rootInstance = _fznInit(argv.data(), argv.size());
      rootInitialized = true;
    }
    // Only claim ownership once the library is fully usable, so the destructor
    // cannot double-free a partially constructed object.
    _library = loaded;
  } catch (...) {
    if (rootInitialized && (_fznFree != nullptr)) {
      try {
        _fznFree(_rootInstance);
      } catch (...) {
      }
    }
    closeLibrary(loaded);
    throw;
  }
}

BlackBoxDLL::~BlackBoxDLL() {
  if ((_fznInit != nullptr) && (_fznFree != nullptr)) {
    for (const Instance& instance : _instances) {
      try {
        _fznFree(instance.value);
      } catch (...) {
      }
    }
    try {
      _fznFree(_rootInstance);
    } catch (...) {
    }
  }
  closeLibrary(_library);
}

void* BlackBoxDLL::instance() {
  const std::thread::id owner = std::this_thread::get_id();
  const std::lock_guard lock(_mutex);
  // Each live thread has exclusive access to its selected instance.
  for (const Instance& instance : _instances) {
    if (instance.owner == owner) {
      return instance.value;
    }
  }
  void* value = _fznClone(_rootInstance);
  try {
    _instances.push_back(Instance{owner, value});
  } catch (...) {
    if (_fznFree != nullptr) {
      try {
        _fznFree(value);
      } catch (...) {
      }
    }
    throw;
  }
  return value;
}

void BlackBoxDLL::run(const std::vector<Int>& intIn,
                      const std::vector<double>& floatIn,
                      std::vector<Int>& intOut, std::vector<double>& floatOut) {
  static_assert(std::is_same_v<Int, int64_t>,
                "the blackbox ABI passes atlantis::Int as int64_t");

  // A library without `fzn_init` carries no state and is assumed reentrant, so
  // it needs no per-thread instance and no locking at all.
  void* state = _fznInit != nullptr ? instance() : nullptr;

  _fznBlackbox(state, intIn.data(), intIn.size(), floatIn.data(),
               floatIn.size(), intOut.data(), intOut.size(), floatOut.data(),
               floatOut.size());

  for (size_t i = 0; i < floatOut.size(); ++i) {
    checkFloat(floatOut[i], i);
  }
}

BlackBoxExec::BlackBoxExec(std::string program, std::vector<std::string> args)
    : _program(std::move(program)), _args(std::move(args)) {}

BlackBoxExec::~BlackBoxExec() {
  const std::lock_guard lock(_mutex);
  for (BlackBoxProcessSession* s : _sessions) {
    delete s;
  }
  _sessions.clear();
}

BlackBoxProcessSession& BlackBoxExec::session() {
  const std::lock_guard lock(_mutex);
  for (BlackBoxProcessSession* s : _sessions) {
    if (s->ownedByCurrentThread()) {
      return *s;
    }
  }
  std::unique_ptr<BlackBoxProcessSession> s(
      createBlackBoxProcess(_program, _args));
  BlackBoxProcessSession* r = s.get();
  _sessions.push_back(r);
  s.release();
  return *r;
}

void BlackBoxExec::run(const std::vector<Int>& intIn,
                       const std::vector<double>& floatIn,
                       std::vector<Int>& intOut,
                       std::vector<double>& floatOut) {
  const std::string response =
      session().exchange(encodeBlackBoxRequest(intIn, floatIn));
  decodeBlackBoxResponse(response, intOut, floatOut);
}

std::string encodeBlackBoxRequest(const std::vector<Int>& intIn,
                                  const std::vector<double>& floatIn) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(std::numeric_limits<double>::max_digits10);
  for (size_t i = 0; i < intIn.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << intIn[i];
  }
  out << ";";
  for (size_t i = 0; i < floatIn.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << floatIn[i];
  }
  out << "\n";
  return out.str();
}

void decodeBlackBoxResponse(const std::string& response,
                            std::vector<Int>& intOut,
                            std::vector<double>& floatOut) {
  if (response.find('\0') != std::string::npos) {
    throw std::runtime_error(
        "BlackBoxExec: blackbox process response contains NUL data");
  }
  // Parse the response in a single left-to-right pass.
  const char* p = response.c_str();
  auto skipWs = [](const char*& q) {
    while (*q == ' ' || *q == '\t' || *q == '\r') {
      ++q;
    }
  };
  auto skipFinalWs = [](const char*& q) {
    while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n') {
      ++q;
    }
  };
  auto valueEnd = [](const char* q) {
    while (*q != ',' && *q != ';' && *q != '\n' && *q != '\0') {
      ++q;
    }
    return q;
  };
  auto checkIntegerTail = [](const char* q, const char* end) {
    while (q != end) {
      if (*q != ' ' && *q != '\t' && *q != '\r') {
        return false;
      }
      ++q;
    }
    return true;
  };
  auto checkNumberTail = [](std::istringstream& in) {
    char c = 0;
    while (in.get(c)) {
      if (c != ' ' && c != '\t' && c != '\r') {
        return false;
      }
    }
    return true;
  };

  for (size_t i = 0; i < intOut.size(); ++i) {
    skipWs(p);
    const char* end = valueEnd(p);
    const char* integer = p;
    if (*integer == '+') {
      ++integer;
    }
    int64_t value = 0;
    const std::from_chars_result parsed = std::from_chars(integer, end, value);
    if ((parsed.ptr == integer) || (parsed.ec != std::errc()) ||
        !checkIntegerTail(parsed.ptr, end)) {
      throw std::runtime_error(
          "BlackBoxExec: failed to read output integer " + std::to_string(i) +
          " from blackbox process output, " + std::to_string(intOut.size()) +
          " integer values were expected");
    }
    intOut[i] = value;
    p = end;
    skipWs(p);
    if (i + 1 < intOut.size()) {
      if (*p != ',') {
        throw std::runtime_error(
            "BlackBoxExec: blackbox process response is missing an integer "
            "output separator");
      }
      ++p;
    }
  }

  skipWs(p);
  if (*p != ';') {
    throw std::runtime_error(
        "BlackBoxExec: blackbox process response is missing the `;' separator "
        "between the integer and floating point outputs");
  }
  ++p;

  for (size_t i = 0; i < floatOut.size(); ++i) {
    skipWs(p);
    const char* end = valueEnd(p);
    std::istringstream in(std::string(p, end));
    in.imbue(std::locale::classic());
    double v = 0.0;
    if (!(in >> v) || !checkNumberTail(in)) {
      throw std::runtime_error(
          "BlackBoxExec: failed to read output float " + std::to_string(i) +
          " from blackbox process output, " + std::to_string(floatOut.size()) +
          " floating point values were expected");
    }
    checkFloat(v, i);
    floatOut[i] = v;
    p = end;
    skipWs(p);
    if (i + 1 < floatOut.size()) {
      if (*p != ',') {
        throw std::runtime_error(
            "BlackBoxExec: blackbox process response is missing a floating "
            "point output separator");
      }
      ++p;
    }
  }

  skipFinalWs(p);
  if (*p != '\0') {
    throw std::runtime_error(
        "BlackBoxExec: blackbox process response contains trailing data");
  }
}

}  // namespace atlantis::blackbox
