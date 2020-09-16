#include "core/varView.hpp"
#include "assert.h"

VarView::VarView(const VarId& t_sourceId)
  : Var(NULL_ID),
    m_sourceId(t_sourceId) {}