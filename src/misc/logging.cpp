#include "misc/logging.hpp"

#ifndef NDEBUG
// Definition for the global log level.
atlantis::logging::LogLevel atlantis::logging::globalLogLevel{
    atlantis::logging::LogLevel::warning};
#endif
