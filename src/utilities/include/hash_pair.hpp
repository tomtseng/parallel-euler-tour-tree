#pragma once

#include <utility>

#include <utilities/include/utils.h>

inline unsigned hashIntPair(const std::pair<unsigned, unsigned>& p) {
  unsigned h{hashInt(p.first)};
  h ^= hashInt(p.second) + 0x9e3779b9 + (h << 6) + (h >> 2);
  return h;
}

// For use in hash containers. For instance:
//   std::unordered_map<std::pair<int, int>, std::string, HashIntPairStruct>
//     int_to_string_map;
struct HashIntPairStruct {
  size_t operator () (const std::pair<int, int> &p) const {
    return hashIntPair(p);
  }
};
