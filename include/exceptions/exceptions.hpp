#include <stdexcept>

class VariableAlreadyDefinedException : virtual public std::runtime_error {
protected:
  int m_errorNumber;
  int m_errorOffset;

public:
  /** Constructor (C++ STL string, int, int).
   *  @param msg The error message
   *  @param t_errorNumber Error number
   *  @param t_errorOffset Error offset
   */
  explicit 
  VariableAlreadyDefinedException(const std::string& msg, int t_errorNumber, int t_errorOffset) :
    std::runtime_error(msg),
    m_errorNumber(t_errorNumber),
    m_errorOffset(t_errorOffset) {}

  /** Destructor.
   *  Virtual to allow for subclassing.
   */
  virtual ~VariableAlreadyDefinedException() {}
  
  /** Returns the error number.
   *  @return #m_errorNumber
   */
  virtual int getErrorNumber() const {
    return m_errorNumber;
  }
  /**Returns the error offset.
   * @return #m_errorOffset
   */
  virtual int getErrorOffset() const {
    return m_errorOffset;
  }
};