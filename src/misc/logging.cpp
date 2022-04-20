#include "misc/logging.hpp"

#ifndef NDEBUG
// Definition for the global log level.
Logging::LogLevel Logging::globalLogLevel{Logging::LogLevel::warning};
#endif
