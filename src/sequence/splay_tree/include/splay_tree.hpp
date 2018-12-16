#pragma once

#include <algorithm>

namespace splay_tree {

class Node {
 public:
  Node();

  // Get first/last element in the tree that this node is in
  Node* GetMin();
  Node* GetMax();
  Node* GetRep();
  void Append(Node* new_node);
  Node* GetSuccessor();
  // Splits right after this node
  std::pair<Node*, Node*> Split();
  static Node* Join(Node* lesser, Node* greater);
  // returns pointer to a node in the same tree (or nullptr if tree is empty)
  Node* DeleteMin();
  Node* DeleteMax();

 private:
  Node* parent;
  Node* child[2];

  Node(Node* _parent, Node* left, Node *right);

  void AssignChild(int i, Node* v);
  // rotate Node towards its parent
  void Rotate();
  void Splay();
  Node* GetExtreme(int i);
  Node* DeleteExtreme(int i);
};

} // namespace splay_tree
