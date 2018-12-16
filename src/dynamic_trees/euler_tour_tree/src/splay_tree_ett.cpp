#include <dynamic_trees/euler_tour_tree/include/splay_tree_ett.hpp>

namespace splay_tree_ett {

using Node = splay_tree::Node;
using std::pair;

EulerTourTree::EulerTourTree(int _num_verts) : num_verts(_num_verts) {
  verts = new Node[num_verts];
  edges.reserve(2 * (num_verts - 1));
}

EulerTourTree::~EulerTourTree() {
  delete[] verts;
  for (auto it : edges)
    delete it.second;
}

bool EulerTourTree::IsConnected(int u, int v) {
  return verts[u].GetMin() == verts[v].GetMin();
}

void EulerTourTree::Link(int u, int v) {
  Node* u_left, * u_right, * v_left, * v_right;
  Node* uv = new Node();
  Node* vu = new Node();
  edges[std::make_pair(u, v)] = uv;
  edges[std::make_pair(v, u)] = vu;
  std::tie(u_left, u_right) = verts[u].Split();
  std::tie(v_left, v_right) = verts[v].Split();
  Node::Join(u_left, uv);
  Node::Join(u_left, v_right);
  Node::Join(u_left, v_left);
  Node::Join(u_left, vu);
  Node::Join(u_left, u_right);
}

void EulerTourTree::Cut(int u, int v) {
  Node* uv_left, * uv_right, * vu_left, * vu_right;
  auto uv_it = edges.find(std::make_pair(u, v));
  auto vu_it = edges.find(std::make_pair(v, u));
  std::tie(uv_left, uv_right) = uv_it->second->Split();
  const bool uv_vu_in_order = uv_left->GetMin() != vu_it->second->GetMin();
  std::tie(vu_left, vu_right) = vu_it->second->Split();
  edges.erase(uv_it);
  edges.erase(vu_it);
  uv_left = uv_left->DeleteMax();
  vu_left = vu_left->DeleteMax();
  if (uv_vu_in_order) {
    Node::Join(uv_left, vu_right);
  } else {
    Node::Join(vu_left, uv_right);
  }
}

bool* EulerTourTree::BatchConnected(pair<int, int>* queries, int len) {
  bool* ans = new bool[len];
  for (int i = 0; i < len; i++) {
    ans[i] = IsConnected(queries[i].first, queries[i].second);
  }
  return ans;
}

void EulerTourTree::BatchLink(pair<int, int>* links, int len) {
  for (int i = 0; i < len; i++) {
    Link(links[i].first, links[i].second);
  }
}

void EulerTourTree::BatchCut(pair<int, int>* cuts, int len) {
  for (int i = 0; i < len; i++) {
    Cut(cuts[i].first, cuts[i].second);
  }
}

} // namespace splay_tree_ett
