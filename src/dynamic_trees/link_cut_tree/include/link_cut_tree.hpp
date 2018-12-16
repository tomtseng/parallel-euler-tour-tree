#pragma once

#include <algorithm>

namespace link_cut_tree {

class Node;

class LinkCutTree {
 public:
  LinkCutTree(int _num_verts);
  ~LinkCutTree();
  
  bool* BatchConnected(std::pair<int, int>* queries, int len);
  // Inserting all links in [links] must keep the graph acylic.
  void BatchLink(std::pair<int, int>* links, int len);
  // All edges in [cuts] must be in the graph, and no edges may be repeated.
  void BatchCut(std::pair<int, int>* cuts, int len);

 private:
  Node* verts;
  int num_verts;
};

} // namespace link_cut_tree
