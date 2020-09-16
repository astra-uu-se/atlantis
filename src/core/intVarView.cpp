#include "core/intVarView.hpp"

#include <limits>

extern Id NULL_ID;

/*
This should probably be changed to some more clever implementation.
For example: std::variant
Or possibly some even smarter way
*/

IntVarView::IntVarView(const VarId& t_sourceId)
    : VarView(t_sourceId),
      m_savedInt(0, 0) {}