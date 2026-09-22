#include "atlantis/misc/logging.hpp"

#ifndef NDEBUG
// Definition for the global log level.
atlantis::logging::LogLevel atlantis::logging::globalLogLevel{warning};
void ::atlantis::logging::setLogLevel(LogLevel newLogLevel) {}
void atlantis::logging::setLogLevel(LogLevel newLogLevel) {}
#endif
