#pragma once

#include <utility>

namespace treap {

class Node {
 public:
  // Running this concurrently may lead to poor randomness.
  Node();
  explicit Node(unsigned random_int);

  Node* GetRoot() const;

  // Splits right after this node
  std::pair<Node*, Node*> Split();
  // Join tree containing lesser to tree containing greater and return root of
  // resulting tree.
  static Node* Join(Node* lesser, Node* greater);

  static void BatchSplit(Node** splits, int len);
  static void BatchJoin(std::pair<Node*, Node*>* joins, int len);

 private:
  void AssignChild(int i, Node* v);
  static Node* JoinRoots(Node* lesser, Node* greater);
  static void BatchJoinRecurse(
      std::pair<Node*, Node*>* joins,
      int len,
      bool* ignored,
      Node** left_roots);

  Node* parent_;
  Node* child_[2];
  unsigned priority_;

  // For batch join
  Node* right_joiner_;
  bool has_left_joiner_;
};

}  // namespace treap
