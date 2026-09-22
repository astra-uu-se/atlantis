#include "atlantis/logging/logger.hpp"

namespace atlantis::logging {

static std::string formatDuration(const std::chrono::nanoseconds duration) {
  const auto seconds = duration.count() / 1'000'000'000;
  const auto milliseconds = duration.count() / 1'000'000;
  const auto microseconds = duration.count() / 1'000;
  const auto nanoseconds = duration.count();

  std::stringstream output;

  if (seconds > 0) {
    output << seconds << "s";
  } else if (milliseconds > 0) {
    output << milliseconds << "ms";
  } else if (microseconds > 0) {
    output << microseconds << "μs";
  } else {
    output << nanoseconds << "ns";
  }

  return output.str();
}

void TimedLogScopeWrapper::begin(Logger& logger, const Level level) {
  logger.log(level, "Starting {}.", _title);
  _startTime = Clock::now();
  logger.increaseIndentation();
}

void TimedLogScopeWrapper::end(Logger& logger, const Level level) {
  const auto endTime = Clock::now();
  const auto duration = endTime - _startTime;
  const auto formattedDuration = formatDuration(
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration));

  logger.decreaseIndentation();
  logger.log(level, "Finished {}; it took {}.", _title, formattedDuration);
}

void IndentedLogScopeWrapper::begin(Logger& logger, const Level level) {
  logger.log(level, "{}", _title);
  logger.increaseIndentation();
}

void IndentedLogScopeWrapper::end(Logger& logger, Level) {
  logger.decreaseIndentation();
}

Logger::Logger(FILE* location, const Level level)
    : _ostream(location), _maxLevel(level) {}

const char* Logger::levelString(const Level level) {
  switch (level) {
    case Level::LVL_ERROR:
      return "ERROR";
    case Level::LVL_WARNING:
      return "WARNING";
    case Level::LVL_INFO:
      return "INFO";
    case Level::LVL_DEBUG:
      return "DEBUG";
    case Level::LVL_TRACE:
      return "TRACE";
  }

  return "";
}

}  // namespace atlantis::logging
