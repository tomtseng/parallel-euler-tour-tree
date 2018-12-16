#include <dynamic_trees/benchmarks/data/src/graph_io.hpp>

#include <fstream>
#include <vector>

typedef long long ll;
using std::vector;

void PrintAdjGraph(vector<vector<ll>> adj_list, std::string filename) {
  std::ofstream file;
  file.open(filename);
  ll n{static_cast<ll>(adj_list.size())};
  ll m{0};
  for (ll i = 0; i < n; i++) {
    m += adj_list[i].size();
  }

  file << "AdjacencyGraph\n" << n << '\n' << m << '\n';
  ll offset{0};
  for (ll i = 0; i < n; i++) {
    file << offset << '\n';
    offset += adj_list[i].size();
  }
  for (auto vec : adj_list) {
    for (auto v : vec) {
      file << v << '\n';
    }
  }

  file.close();
}

