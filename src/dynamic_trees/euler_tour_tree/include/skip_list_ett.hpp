#pragma once

#include <algorithm>
#include <unordered_map>
#include <vector>

#include <sequence/skip_list/include/skip_list.hpp>
#include <utilities/include/hash_pair.hpp>

namespace skip_list_ett {

class EulerTourTree {
 public:
  EulerTourTree(int _num_verts);
  // Note: this calls [finish()] on a [list_allocator<ETElement>]. Since
  // [list_allocator<>] functions are all static, this may cause errors if any
  // other [list_allocator<ETElement>]s are around in use (e.g. by other
  // [EulerTourTree] structures)
  // TODO(ttseng) deal with aforementioned issue better
  ~EulerTourTree();
  // Constructors and operators for C++11's rule of 5
  // TODO(ttseng) unimplemented
  EulerTourTree(const EulerTourTree&) = delete;
  EulerTourTree(EulerTourTree&&) = delete;
  EulerTourTree& operator=(const EulerTourTree&) = delete;
  EulerTourTree& operator=(EulerTourTree&&) = delete;

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
  skip_list::Element* verts;
  std::unordered_map<std::pair<int, int>, skip_list::Element*,
    HashIntPairStruct> edges;
  std::vector<skip_list::Element*> node_pool;
};

} // namespace skip_list_ett
