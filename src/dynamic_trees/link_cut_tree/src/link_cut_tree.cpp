#include <dynamic_trees/link_cut_tree/include/link_cut_tree.hpp>

namespace link_cut_tree {

class Node {
 public:
  Node();
  Node(Node* _par, Node* left, Node *right);

  Node* get_root();
  void cut(Node* neighbor);
  void cut_from_par();
  void link(Node* child);
  Node* lca(Node* other);
  void evert(); // reroot
  int path_to_root_query();

 private:
  Node* par; // parent
  Node* c[2]; // children
  bool flip; // whether children are reversed; used for evert()

  Node* get_real_par();
  void rot();
  void splay();
  Node* expose();
  void fix_c();
  void push_flip();
};

Node::Node(Node* _par, Node* left, Node *right)
  : par(_par), c{left, right}, flip(0) {
  fix_c();
}

Node::Node() : Node(nullptr, nullptr, nullptr) {}

Node* Node::get_real_par() {
  return par != nullptr && this != par->c[0] && this != par->c[1] ? nullptr : par;
}

void Node::fix_c() {
  for (int i = 0; i < 2; i++)
    if (c[i] != nullptr)
      c[i]->par = this;
}

void Node::push_flip() {
  if (flip) {
    flip = 0;
    std::swap(c[0], c[1]);
    for (int i = 0; i < 2; i++)
      if (c[i] != nullptr)
        c[i]->flip ^= 1;
  }
}

void Node::rot() { // rotate v towards its parent; v must have real parent
  Node* p = get_real_par();
  par = p->par;
  if (par != nullptr)
    for (int i = 0; i < 2; i++)
      if (par->c[i] == p) {
        par->c[i] = this;
        par->fix_c();
      }
  const bool rot_dir = this == p->c[0];
  p->c[!rot_dir] = c[rot_dir];
  c[rot_dir] = p;
  p->fix_c();
  fix_c();
}

void Node::splay() {
  Node* p, * gp;
  push_flip(); // guarantee flip bit isn't set after calling splay()
  while ((p = get_real_par()) != nullptr) {
    gp = p->get_real_par();
    if (gp != nullptr)
      gp->push_flip();
    p->push_flip();
    push_flip();
    if (gp != nullptr)
      ((gp->c[0] == p) == (p->c[0] == this) ? p : this)->rot();
    rot();
  }
}

// returns 1st vertex encountered that was originally in same path as root (used
// for LCA)
Node* Node::expose() {
  Node* ret = this;
  for (Node* curr = this, * pref = nullptr; curr != nullptr;
       ret = curr, pref = this, curr = par) {
    curr->splay();
    curr->c[1] = pref;
    curr->fix_c();
    splay();
  }
  return ret;
}

void Node::evert() {
  expose();
  flip ^= 1;
  push_flip();
}

Node* Node::get_root() {
  expose();
  Node* root = this;
  push_flip();
  while (root->c[0] != nullptr) {
    root = root->c[0];
    root->push_flip();
  }
  root->splay();
  return root;
}

void Node::cut_from_par() {
  expose();
  c[0] = c[0]->par = nullptr;
  fix_c();
}

void Node::cut(Node* neighbor) {
  neighbor->evert();
  evert();
  neighbor->par = nullptr;
  for (int i = 0; i < 2; i++)
    if (c[i] == neighbor)
      c[i] = nullptr;
  fix_c();
}

void Node::link(Node* child) {
  child->evert();
  expose();
  child->par = this;
}

Node* Node::lca(Node* other) {
  expose();
  return other->expose();
}

LinkCutTree::LinkCutTree(int _num_verts) : num_verts(_num_verts) {
  verts = new Node[num_verts];
}

LinkCutTree::~LinkCutTree() {
  delete[] verts;
}

bool* LinkCutTree::BatchConnected(std::pair<int, int>* queries, int len) {
  bool* ans = new bool[len];
  for (int i = 0; i < len; i++)
    ans[i] = verts[queries[i].first].get_root() == verts[queries[i].second].get_root();
  return ans;
}

void LinkCutTree::BatchLink(std::pair<int, int>* links, int len) {
  for (int i = 0; i < len; i++)
    verts[links[i].first].link(&verts[links[i].second]);
}

void LinkCutTree::BatchCut(std::pair<int, int>* cuts, int len) {
  for (int i = 0; i < len; i++)
    verts[cuts[i].first].cut(&verts[cuts[i].second]);
}

} // namespace link_cut_tree
