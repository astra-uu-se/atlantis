#include <stdexcept>

class VariableAlreadyDefinedException : public std::runtime_error {

public:
  /** 
   * @param msg The error message
   */
  explicit 
  VariableAlreadyDefinedException(const std::string& msg) :
    std::runtime_error(msg) {}

  /** Destructor.
   */
  ~VariableAlreadyDefinedException() {}
};