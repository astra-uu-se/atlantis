#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "../core/types.hpp"
#include "core/idMap.hpp"
#include "../misc/logging.hpp"

class PropagationGraph {
 protected:
  size_t m_numInvariants;
  size_t m_numVariables;

  /**
   * Map from VarID -> InvariantId
   *
   * Maps to nullptr if not defined by any invariant.
   */
  IdMap<VarIdBase, InvariantId> m_definingInvariant;

  /**
   * Map from InvariantId -> list of VarId
   *
   * Maps an invariant to all variables it defines.
   */
  IdMap<InvariantId, std::vector<VarIdBase>> m_variablesDefinedByInvariant;
  /**
   * Map from InvariantId -> list of VarId
   *
   * Maps an invariant to all variables it depends on (its inputs).
   */
  IdMap<InvariantId, std::vector<VarIdBase>> m_inputVariables;

  // Map from VarId -> vector of InvariantId
  IdMap<VarIdBase, std::vector<InvariantId>> m_listeningInvariants;

  std::vector<bool> m_isOutputVar;
  std::vector<bool> m_isInputVar;

  struct Topology {
    std::vector<size_t> m_variablePosition;
    std::vector<size_t> m_invariantPosition;

    PropagationGraph& graph;
    Topology() = delete;
    explicit Topology(PropagationGraph& g) : graph(g) {}
    void computeNoCycles();
    void computeWithCycles();
    void computeInvariantFromVariables();
    inline size_t getPosition(VarIdBase id) {
      return m_variablePosition[id.id];
    }
    inline size_t getPosition(InvariantId id) {
      return m_invariantPosition.at(id);
    }
  } m_topology;

  friend class PropagationEngine;

  struct PriorityCmp {
    PropagationGraph& graph;
    explicit PriorityCmp(PropagationGraph& g) : graph(g) {}
    bool operator()(VarIdBase left, VarIdBase right) {
      return graph.m_topology.getPosition(left) >
             graph.m_topology.getPosition(right);
    }
  };

  // TODO: Move into its own file:
  class PropagationQueue {
   private:
    struct ListNode {
      size_t priority;
      VarIdBase id;
      ListNode* next;
      ListNode(VarIdBase t_id, size_t t_priority)
          : priority(t_priority), id(t_id), next(nullptr) {}
    };

    IdMap<VarIdBase, std::unique_ptr<ListNode>> m_priorityNodes;
    ListNode* head;
    ListNode* tail;

   public:
    PropagationQueue() : m_priorityNodes(0), head(nullptr), tail(nullptr) {}

    // vars must be initialised in order.
    void initVar(VarIdBase id, size_t priority) {
      assert(!m_priorityNodes.has_idx(id));
      m_priorityNodes.register_idx(id);
      m_priorityNodes[id] = std::make_unique<ListNode>(id, priority);
    }

    bool empty() { return head == nullptr; }
    void push(VarIdBase id) {
      ListNode* toInsert = m_priorityNodes[id].get();
      if (toInsert->next != nullptr || tail == toInsert) {
        return;  // id is already is list
      }
      if (head == nullptr) {
        head = toInsert;
        tail = head;
        return;
      }
      // Insert at start of list
      if (toInsert->priority <=
          head->priority) {  // duplicates should not happen but are ok
        toInsert->next = head;
        head = toInsert;
        return;
      }

      // Insert at end of list
      if (toInsert->priority >=
          tail->priority) {  // duplicates should not happen but are ok
        tail->next = toInsert;
        tail = toInsert;
        return;
      }
      ListNode* current = head;
      while (current->next != nullptr) {
        if (current->next->priority >= toInsert->priority) {
          toInsert->next = current->next;
          current->next = toInsert;
          return;
        }
        current = current->next;
      }
      assert(false);  // Insert failed. Cannot happen.
    }
    VarIdBase pop() {
      if (head == nullptr) {
        return NULL_ID;
      }
      ListNode* ret = head;
      if (head == tail) {
        tail = nullptr;
      }
      head = head->next;
      ret->next = nullptr;
      return ret->id;
    }
    VarIdBase top() {
      if (head == nullptr) {
        return NULL_ID;
      }
      return head->id;
    }
  };

  PropagationQueue m_propagationQueue;
  // std::priority_queue<VarIdBase, std::vector<VarIdBase>,
  //                     PropagationGraph::PriorityCmp>
  //     m_propagationQueue;

 public:
  PropagationGraph() : PropagationGraph(1000) {}
  explicit PropagationGraph(size_t expectedSize);

  /**
   * update internal datastructures based on currently registered  variables and
   * invariants.
   */
  void close();

  /**
   * Register an invariant in the propagation graph.
   */
  void registerInvariant(InvariantId);

  /**
   * Register a variable in the propagation graph.
   */
  void registerVar(VarIdBase);

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param depends the invariant that the variable depends on
   * @param source the depending variable
   */
  void registerInvariantDependsOnVar(InvariantId depends, VarIdBase source);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param depends the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarIdBase depends, InvariantId source);

  [[nodiscard]] inline size_t getNumVariables() const {
    return m_numVariables;  // this ignores null var
  }

  [[nodiscard]] inline size_t getNumInvariants() const {
    return m_numInvariants;  // this ignores null invariant
  }

  inline bool isOutputVar(VarIdBase id) { return m_isOutputVar.at(id); }

  inline bool isInputVar(VarIdBase id) { return m_isInputVar.at(id); }

  inline InvariantId getDefiningInvariant(VarIdBase v) {
    // Returns NULL_ID is not defined.
    return m_definingInvariant.at(v);
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& getVariablesDefinedBy(
      InvariantId inv) const {
    return m_variablesDefinedByInvariant.at(inv);
  }
};
