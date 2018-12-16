// Slow link and cut, fast connectivity queries.
#pragma once

#include <unordered_set>
#include <vector>

class SimpleForestConnectivity {
 public:
  SimpleForestConnectivity() = delete;
  explicit SimpleForestConnectivity(int num_vertices);

  void Link(int u, int v);
  void Cut(int u, int v);
  bool IsConnected(int u, int v) const;

 private:
  int MinimumVertexInComponent(int v);
  void AssignIdToComponent(int v, int id);

  int num_vertices_;
  std::vector<std::unordered_set<int>> adjacency_list_;
  std::vector<int> component_ids_;
  std::vector<bool> visited_;
};
