#pragma once
#ifdef _DEBUG
#ifndef NDEBUG
#define NDEBUG
#endif
#endif
#ifndef NDEBUG
#include <iostream>

namespace atlantis::logging {

enum LogLevel { debug = 0, info = 1, warning = 2, error = 3 };
extern LogLevel globalLogLevel;

inline void setLogLevel(LogLevel newLogLevel) { globalLogLevel = newLogLevel; }

struct None {};

template <typename First, typename Second>
struct Pair {
  First first;
  Second second;
};

template <typename List>
struct LogData {
  List list;
};

template <typename Begin, typename Value>
LogData<Pair<Begin, const Value &>> operator<<(LogData<Begin> begin,
                                               const Value &value) {
  return {{begin.list, value}};
}

template <typename Begin, size_t n>
LogData<Pair<Begin, const char *>> operator<<(LogData<Begin> begin,
                                              const char (&value)[n]) {
  return {{begin.list, value}};
}

inline void printList(std::ostream &, None) {}

template <typename Begin, typename Last>
void printList(std::ostream &os, const Pair<Begin, Last> &data) {
  printList(os, data.first);
  os << data.second;
}

template <typename List>
void log(LogLevel logLevel, const char *file, int line,
         const LogData<List> &data) {
  if (globalLogLevel > logLevel) {
    return;
  }
  const char *level;
  switch (logLevel) {
    case LogLevel::error:
      level = "[error] ";
      break;
    case LogLevel::warning:
      level = "[warning] ";
      break;
    case LogLevel::info:
      level = "[info] ";
      break;
    default:
      level = "[debug] ";
  }

  std::cerr << level << file << " (" << line << "): ";
  printList(std::cerr, data.list);
  std::cerr << '\n';
}
}  // namespace atlantis::logging
#define setLogLevel(x) \
  (atlantis::logging::setLogLevel(atlantis::logging::LogLevel::x))
#define logDebug(x)                                           \
  (atlantis::logging::log(                                    \
      atlantis::logging::LogLevel::debug, __FILE__, __LINE__, \
      atlantis::logging::LogData<atlantis::logging::None>() << x))
#define logInfo(x)                                           \
  (atlantis::logging::log(                                   \
      atlantis::logging::LogLevel::info, __FILE__, __LINE__, \
      atlantis::logging::LogData<atlantis::logging::None>() << x))
#define logWarning(x)                                           \
  (atlantis::logging::log(                                      \
      atlantis::logging::LogLevel::warning, __FILE__, __LINE__, \
      atlantis::logging::LogData<atlantis::logging::None>() << x))
#define logError(x)                                           \
  (atlantis::logging::log(                                    \
      atlantis::logging::LogLevel::error, __FILE__, __LINE__, \
      atlantis::logging::LogData<atlantis::logging::None>() << x))
#else
#define setLogLevel(x) \
  do {                 \
  } while (0)
#define logDebug(x) \
  do {              \
  } while (0)
#define logInfo(x) \
  do {             \
  } while (0)
#define logWarning(x) \
  do {                \
  } while (0)
#define logError(x) \
  do {              \
  } while (0)
#endif