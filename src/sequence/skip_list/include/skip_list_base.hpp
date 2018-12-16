// This is copied over from parallel_skip_list/.
#pragma once
#include <utilities/include/random.h>

namespace skip_list {

template <typename T>
struct Neighbors {
  T* prev;
  T* next;
  Neighbors() : prev(nullptr), next(nullptr) {};
};

template <typename Derived>
class ElementBase {
 public:
  ElementBase();
  ElementBase(size_t random_int);

  // Deletes this node, not all nodes in the list.
  virtual ~ElementBase();
  // Constructors and operators for C++11's rule of 5
  // TODO(tomtseng) unimplemented
  ElementBase(const ElementBase&) = delete;
  ElementBase(ElementBase&&) = delete;
  ElementBase& operator=(const ElementBase&) = delete;
  ElementBase& operator=(ElementBase&&) = delete;

  Derived* GetPreviousElement() const;
  Derived* GetNextElement() const;

  // Returns a representative node from the list the node lives in.
  // A representative node is only guaranteed to be valid until the next
  // [Join] or [Split] call.
  Derived* FindRepresentative();

  // Concatenates the list that [left] lives in to the list that [right] lives in.
  //
  // [left] must be the last node in its list. [right] must be the first node
  // in its list.
  static void Join(Derived* left, Derived* right);

  // Split the list right after this node (so this node will be the last node
  // in its list). Returns what was the successor to the node (the first node
  // on the right-hand side of the split).
  Derived* Split();

 protected:
  // neighbors[i] holds neighbors at level i, where level 0 is the lowest level
  // and is the level at which the list contains all elements
  Neighbors<Derived>* neighbors;
  int height;

  static pbbs::random default_random;

  // When called on node [v], searches left starting from and including [v]
  // for the first node at the next level up.
  Derived* FindLeftParent(int level);
  // When called on node [v], searches right starting from and including [v]
  // for the first node at the next level up.
  Derived* FindRightParent(int level);

  static int RandomIntToHeight(size_t rand_int);
};

template <typename Derived>
pbbs::random ElementBase<Derived>::default_random;

template <typename Derived>
int ElementBase<Derived>::RandomIntToHeight(size_t random_int) {
  int h = 1;
  // Geometric(1/2) distribution.
  // Note: could also consider Geometric(3/4) (so 1/4 of nodes go up to
  // next level) by reading two bits at once
  while (random_int & 1) {
    random_int >>= 1;
    h++;
  }
  return h;
}

template <typename Derived>
ElementBase<Derived>::ElementBase() {
  size_t random_int = default_random.rand();
  default_random = default_random.next(); // race
  height = RandomIntToHeight(random_int);
  neighbors = new Neighbors<Derived>[height];
}

template <typename Derived>
ElementBase<Derived>::ElementBase(size_t random_int) {
  height = RandomIntToHeight(random_int);
  neighbors = new Neighbors<Derived>[height];
}

template <typename Derived>
ElementBase<Derived>::~ElementBase() {
  delete[] neighbors;
}

template <typename Derived>
Derived* ElementBase<Derived>::GetPreviousElement() const {
  return neighbors[0].prev;
}

template <typename Derived>
Derived* ElementBase<Derived>::GetNextElement() const {
  return neighbors[0].next;
}

template <typename Derived>
Derived* ElementBase<Derived>::FindLeftParent(int level) {
  Derived* current_node = static_cast<Derived*>(this);
  Derived* start_node = current_node;
  do {
    if (current_node->height > level + 1) {
      return current_node;
    }
    current_node = current_node->neighbors[level].prev;
  } while (current_node != nullptr && current_node != start_node);
  return nullptr;
}

template <typename Derived>
Derived* ElementBase<Derived>::FindRightParent(int level) {
  Derived* current_node = static_cast<Derived*>(this);
  Derived* start_node = current_node;
  do {
    if (current_node->height > level + 1) {
      return current_node;
    }
    current_node = current_node->neighbors[level].next;
  } while (current_node != nullptr && current_node != start_node);
  return nullptr;
}

// If the list is cyclic, return node on highest level, breaking ties in favor
// of the lowest address.
// If the list is not cyclic, then return the head node on the highest level.
template <typename Derived>
Derived* ElementBase<Derived>::FindRepresentative() {
  Derived* current_node = static_cast<Derived*>(this);
  Derived* seen_node = nullptr;
  int current_level = current_node->height - 1;

  // walk up while moving forward
  while (current_node->neighbors[current_level].next != nullptr &&
         seen_node != current_node) {
    if (seen_node == nullptr || current_node < seen_node) {
      seen_node = current_node;
    }
    current_node = current_node->neighbors[current_level].next;
    const int top_level = current_node->height - 1;
    if (current_level < top_level) {
      current_level = top_level;
      seen_node = nullptr;
    }
  }

  if (seen_node == current_node) { // list is a cycle
    return seen_node;
  } else {
    // walk up while moving backward
    while (current_node->neighbors[current_level].prev != nullptr) {
      current_node = current_node->neighbors[current_level].prev;
      current_level = current_node->height - 1;
    }
    return current_node;
  }
}

template <typename Derived>
void ElementBase<Derived>::Join(Derived* left, Derived* right) {
  int level = 0;
  while (left != nullptr && right != nullptr) {
    left->neighbors[level].next = right;
    right->neighbors[level].prev = left;
    left = left->FindLeftParent(level);
    right = right->FindRightParent(level);
    level++;
  }
}

template <typename Derived>
Derived* ElementBase<Derived>::Split() {
  Derived* successor = GetNextElement();
  Derived* current_node = static_cast<Derived*>(this);
  int level = 0;
  Derived* next;
  while (current_node != nullptr &&
      (next = current_node->neighbors[level].next) != nullptr) {
    current_node->neighbors[level].next = nullptr;
    next->neighbors[level].prev = nullptr;
    current_node = current_node->FindLeftParent(level);
    level++;
  }
  return successor;
}

} // namespace skip_list
