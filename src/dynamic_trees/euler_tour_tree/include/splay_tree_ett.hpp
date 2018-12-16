#pragma once

#include <algorithm>
#include <unordered_map>

#include <sequence/splay_tree/include/splay_tree.hpp>
#include <utilities/include/hash_pair.hpp>

namespace splay_tree_ett {

class EulerTourTree {
 public:
  EulerTourTree(int _num_verts);
  ~EulerTourTree();
  EulerTourTree(const EulerTourTree& other);
  EulerTourTree& operator=(const EulerTourTree& other);

  bool IsConnected(int u, int v);
  void Link(int u, int v);
  void Cut(int u, int v);

  bool* BatchConnected(std::pair<int, int>* queries, int len);
  // Inserting all links in [links] must keep the graph acylic.
  void BatchLink(std::pair<int, int>* links, int len);
  // All edges in [cuts] must be in the graph, and no edges may be repeated.
  void BatchCut(std::pair<int, int>* cuts, int len);

 private:
  int num_verts;
  splay_tree::Node* verts;
  std::unordered_map<std::pair<int, int>, splay_tree::Node*, HashIntPairStruct> edges;
};

} //namespace splay_tree_ett
