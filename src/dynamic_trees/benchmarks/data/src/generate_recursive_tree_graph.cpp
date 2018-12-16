#include <random>
#include <vector>

#include <dynamic_trees/benchmarks/data/src/graph_io.hpp>
#include <utilities/include/parse_command_line.h>

typedef long long ll;
using std::vector;

// An n-vertex recursive tree is generated as follows:
// for i = 1, 2, 3, ..., n - 1, draw integer j from [0,  i - 1) uniformly at
// random and add edge {i, j} to the graph.
int main(int argc, char* argv[]) {
  commandLine P{argc, argv, "n <outFile>"};
  pair<ll,char*> in{P.sizeAndFileName()};
  ll n{in.first};
  char* filename{in.second};

  vector<vector<ll>> adj_list(n);
  std::mt19937 generator{0};
  for (ll i = 1; i < n; i++) {
    std::uniform_int_distribution<ll> distribution{0, i - 1};
    ll parent{distribution(generator)};
    adj_list[i].push_back(parent);
    adj_list[parent].push_back(i);
  }

  PrintAdjGraph(adj_list, string(filename));
  return 0;
}
