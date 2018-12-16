#include <sequence/splay_tree/include/splay_tree.hpp>

using std::pair;

namespace splay_tree {

Node::Node(Node* _parent, Node* left, Node *right)
  : parent(_parent), child{left, right} {}

Node::Node() : Node(nullptr, nullptr, nullptr) {}

void Node::AssignChild(int i, Node* v) {
  if (v != nullptr) {
    v->parent = this;
  }
  child[i] = v;
}

void Node::Rotate() {
  Node* p = parent;
  parent = p->parent;
  if (parent != nullptr) {
    for (int i = 0; i < 2; i++) {
      if (parent->child[i] == p) {
        parent->child[i] = this;
      }
    }
  }
  const bool rotate_direction = this == p->child[0];
  p->AssignChild(!rotate_direction, child[rotate_direction]);
  AssignChild(rotate_direction, p);
}

void Node::Splay() {
  Node* p = parent;
  while (p != nullptr) {
    Node* pp = p->parent;
    if (pp != nullptr) {
      const bool is_zigzig = (pp->child[0] == p) == (p->child[0] == this);
      if (is_zigzig) {
        p->Rotate();
      } else {
        Rotate();
      }
    }
    Rotate();
    p = parent;
  }
}

Node* Node::GetExtreme(int i) {
  Splay();
  Node* cur = this;
  while (cur->child[i] != nullptr)
    cur = cur->child[i];
  cur->Splay();
  return cur;
}

Node* Node::GetMin() {
  return GetExtreme(0);
}

Node* Node::GetMax() {
  return GetExtreme(1);
}

Node* Node::GetRep() {
  return GetMin();
}

Node* Node::GetSuccessor() {
  Splay();
  if (child[1] == nullptr)
    return nullptr;
  Node* cur = child[1];
  while (cur->child[0] != nullptr)
    cur = cur->child[0];
  return cur;
}

pair<Node*, Node*> Node::Split() {
  Splay();
  Node* right = child[1];
  if (right != nullptr)
    right->parent = nullptr;
  child[1] = nullptr;
  return std::make_pair(this, right);
}

Node* Node::Join(Node* lesser, Node* greater) {
  if (lesser == nullptr)
    return greater;
  if (greater == nullptr)
    return lesser;
  lesser->Splay();
  greater->Splay();
  Node* lesser_max = lesser->GetMax();
  lesser_max->AssignChild(1, greater);
  return lesser_max;
}

void Node::Append(Node* new_node) {
  GetMax()->AssignChild(1, new_node);
}

Node* Node::DeleteExtreme(int i) {
  Node* to_delete = GetExtreme(i);
  Node* tree_pointer = to_delete->child[!i];
  if (tree_pointer != nullptr)
    tree_pointer->parent = nullptr;
  delete to_delete;
  return tree_pointer;
}

Node* Node::DeleteMin() {
  return DeleteExtreme(0);
}

Node* Node::DeleteMax() {
  return DeleteExtreme(1);
}

} // namespacesplay_tree
