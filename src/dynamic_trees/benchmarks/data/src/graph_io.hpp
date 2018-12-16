#pragma once

#include <string>
#include <vector>

// Prints adjacency list in PBBS adjacency graph format
void PrintAdjGraph(
    std::vector<std::vector<long long>> adj_list, std::string filename);
