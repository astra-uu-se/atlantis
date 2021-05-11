#pragma once

struct PropagationListNode {
  size_t priority;
  VarIdBase id;
  PropagationListNode* next;

  PropagationListNode() : PropagationListNode(NULL_ID, 0) {}
  PropagationListNode(VarIdBase t_id, size_t t_priority)
      : priority(t_priority), id(t_id), next(nullptr) {}
};