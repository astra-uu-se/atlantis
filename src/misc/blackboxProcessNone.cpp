#include "atlantis/misc/blackboxProcess.hpp"

// Fallback used when neither the POSIX nor the Windows implementation is
// available for this platform.
#if !defined(_WIN32) && !defined(ATLANTIS_HAS_POSIX_BLACKBOX_EXEC)

#include <stdexcept>
#include <string>
#include <vector>

namespace atlantis::blackbox {

BlackBoxProcessSession* createBlackBoxProcess(
    const std::string&, const std::vector<std::string>&) {
  throw std::runtime_error(
      "BlackBoxExec: executable blackbox propagators are not supported on this "
      "platform");
}

}  // namespace atlantis::blackbox

#endif
