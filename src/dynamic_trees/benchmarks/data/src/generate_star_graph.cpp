#include <vector>

#include <dynamic_trees/benchmarks/data/src/graph_io.hpp>
#include <utilities/include/parse_command_line.h>

typedef long long ll;
using std::vector;

int main(int argc, char* argv[]) {
  commandLine P(argc, argv, "n <outFile>");
  pair<ll,char*> in{P.sizeAndFileName()};
  ll n{in.first};
  char* filename = in.second;

  vector<vector<ll>> adj_list(n);
  for (ll i = 1; i < n; i++) {
    adj_list[0].push_back(i);
    adj_list[i].push_back(0);
  }

  PrintAdjGraph(adj_list, string(filename));
  return 0;
}
