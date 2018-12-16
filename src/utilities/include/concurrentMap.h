#pragma once

#include <tuple>
#include "utils.h"

using namespace std;
using utils::_seq;

namespace concurrent_map {

// Thresholds for resizing down and up respectively
constexpr const double _load_lb = 0.05;
constexpr const double _load_ub = 0.95;
constexpr const double _growth_ratio = 2;
constexpr const size_t _minimum_ht_size = 4;

template <class K, class V, class HashFn>
class concurrentHT {
public:
  using KV = tuple<K, V>;
  HashFn H;
  KV* table;
  K empty_key;
  K tombstone;
  bool alloc;

  size_t n_elms;
  size_t n_tombstones;
  size_t capacity;
  size_t mask;

  inline void clearA(KV* v, size_t n, K emp_key) {
    granular_for(i, 0, n, (n > 2000), { get<0>(v[i]) = emp_key; });
  }

  inline size_t toRange(size_t h) const {return h & mask;}
  inline size_t firstIndex(K k) const {return toRange(H(k));}
  inline size_t incrementIndex(size_t h) const {return toRange(h+1);}

  inline KV* alloc_table(size_t _m) {
    // Must initialize for std::function()
    KV* tab = newA(KV, _m);
    clearA(tab, _m, empty_key);
    return tab;
  }

  inline void del() {
    if (alloc) {
      free(table);
    }
  }

  concurrentHT() : capacity(0), mask(0), table(0), n_elms(0), alloc(false) { }

  // Assumes size is a power of two.
  concurrentHT(KV* _table, size_t size, K _ek, K _ts) :
    table(_table), empty_key(_ek), tombstone(_ts), alloc(false), n_elms(0),
    n_tombstones(0), capacity(size), mask(size-1) {
      if (table == nullptr) {
        capacity = 1 << pbbs::log2_up(100+(intT)(1.1*(float)size));
        mask = capacity-1;
        table = alloc_table(capacity);
        alloc=true;
      }
    }

  inline maybe<V> find(K k) const {
    size_t h = firstIndex(k);
    while(1) {
      KV t_kv = table[h];
      K t_k = get<0>(t_kv);
      if (t_k == k) {
        return maybe<V>(get<1>(t_kv));
      } else if (t_k == empty_key) {
        return maybe<V>();
      }
      h = incrementIndex(h);
    }
  }

  // Phase concurrent
  inline bool insert(K k, V v) {
    size_t h = firstIndex(k);
    while(1) {
      KV t_kv = table[h];
      K t_k = get<0>(t_kv);
      if ((t_k == empty_key && CAS(&(get<0>(table[h])), empty_key, k)) ||
          (t_k == tombstone && CAS(&(get<0>(table[h])), tombstone, k))) {
        get<1>(table[h]) = v;
        return true;
      } else if (t_k == k) {
        return false;
      }
      h = incrementIndex(h);
    }
  }

  // Phase concurrent
  inline bool deleteVal(K k) {
    size_t h = firstIndex(k);
    while(1) {
      KV t_kv = table[h];
      K t_k = get<0>(t_kv);
      if (t_k == empty_key) {
        return false;
      } else if (t_k == k) {
        // No atomics necessary?
        get<0>(table[h]) = tombstone;
        return true;
      }
      h = incrementIndex(h);
    }
  }
};

}; // namespace
