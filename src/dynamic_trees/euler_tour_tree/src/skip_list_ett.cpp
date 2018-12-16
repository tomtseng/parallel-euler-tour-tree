#include <dynamic_trees/euler_tour_tree/include/skip_list_ett.hpp>

#include <utilities/include/random.h>

namespace skip_list_ett {

using Element = skip_list::Element;
using std::pair;

EulerTourTree::EulerTourTree(int _num_verts) : num_verts(_num_verts) {
  pbbs::random randomness;
  verts = pbbs::new_array_no_init<Element>(num_verts);
  for (int i = 0; i < num_verts; i++) {
    new (&verts[i]) Element(randomness.ith_rand(i));
    Element::Join(&verts[i], &verts[i]);
  }
  randomness = randomness.next();
  const int max_edges = 2 * (num_verts - 1);
  edges.reserve(max_edges);
  node_pool.reserve(max_edges);
  for (int i = 0; i < max_edges; i++) {
    node_pool.push_back(new Element(randomness.ith_rand(i)));
  }
}

EulerTourTree::~EulerTourTree() {
  for (auto it : edges) {
    delete it.second;
  }
  for (auto it : node_pool) {
    delete it;
  }
  pbbs::delete_array(verts, num_verts);
}

bool EulerTourTree::IsConnected(int u, int v) {
  return verts[u].FindRepresentative() == verts[v].FindRepresentative();
}

void EulerTourTree::Link(int u, int v) {
  Element* uv = node_pool.back();
  node_pool.pop_back();
  Element* vu = node_pool.back();
  node_pool.pop_back();
  edges[make_pair(u, v)] = uv;
  edges[make_pair(v, u)] = vu;
  Element* u_left = &verts[u];
  Element* v_left = &verts[v];
  Element* u_right = u_left->Split();
  Element* v_right = v_left->Split();
  Element::Join(u_left, uv);
  Element::Join(uv, v_right);
  Element::Join(v_left, vu);
  Element::Join(vu, u_right);
}

void EulerTourTree::Cut(int u, int v) {
  auto uv_it = edges.find(std::make_pair(u, v));
  auto vu_it = edges.find(std::make_pair(v, u));
  Element* uv = uv_it->second;
  Element* vu = vu_it->second;
  edges.erase(uv_it);
  edges.erase(vu_it);
  Element* u_left = uv->GetPreviousElement();
  Element* v_left = vu->GetPreviousElement();
  Element* v_right = uv->Split();
  Element* u_right = vu->Split();
  u_left->Split();
  v_left->Split();
  Element::Join(u_left, u_right);
  Element::Join(v_left, v_right);
  node_pool.push_back(uv);
  node_pool.push_back(vu);
}

bool* EulerTourTree::BatchConnected(pair<int, int>* queries, int len) {
  bool* ans = pbbs::new_array_no_init<bool>(len);
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

} // namespace skip_list_ett
