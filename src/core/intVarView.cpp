#include "core/intVarView.hpp"

#include <limits>

extern Id NULL_ID;

/*
This should probably be changed to some more clever implementation.
For example: std::variant
Or possibly some even smarter way
*/

IntVarView::IntVarView(const VarId& t_parentId)
    : VarView(t_parentId),
      m_savedInt(0, 0) {}