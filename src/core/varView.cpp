#include "core/varView.hpp"
#include "assert.h"

VarView::VarView(const VarId& t_parentId)
  : Var(NULL_ID),
    m_parentId(t_parentId) {
      m_id.idType = VarIdType::view;
    }