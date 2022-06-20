#include "logging/logger.hpp"

#include <sstream>

using namespace logging;

static std::string formatDuration(std::chrono::nanoseconds duration) {
  auto seconds = duration.count() / 1'000'000'000;
  auto milliseconds = duration.count() / 1'000'000;
  auto microseconds = duration.count() / 1'000;
  auto nanoseconds = duration.count();

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

void TimedLogScopeWrapper::begin(Logger& logger, Level level) {
  logger.log(level, "Starting {}.", _title);
  _startTime = Clock::now();
  logger.increaseIndentation();
}

void TimedLogScopeWrapper::end(Logger& logger, Level level) {
  auto endTime = Clock::now();
  auto duration = endTime - _startTime;
  auto formattedDuration = formatDuration(
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration));

  logger.decreaseIndentation();
  logger.log(level, "Finished {}; it took {}.", _title,
             formattedDuration);
}

void IndentedLogScopeWrapper::begin(Logger& logger, Level level) {
  logger.log(level, "{}", _title);
  logger.increaseIndentation();
}

void IndentedLogScopeWrapper::end(Logger& logger, Level) {
  logger.decreaseIndentation();
}
