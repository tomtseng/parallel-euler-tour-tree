#pragma once

#include <sequence/parallel_skip_list/include/concurrent_array_allocator.hpp>
#include <utilities/include/random.h>
#include <utilities/include/utils.h>

namespace parallel_skip_list {

// This is the base implementation of a phase-concurrent skip list supporting
// splits and joins.
//
// We want to be able to have derived classes that add their own data members to
// each element of the skip list. To do this, we use the curiously recurring
// template pattern (CRTP). By passing in the derived class through a template,
// the skip list can hold pointers to the instances of derived class. A minimal
// instantiation is given in "skip_list.hpp".
//
// Derived classes must provide a (perhaps empty) `DerivedInitialize()` and
// `DerivedFinish()` function, which will be called in `Initialize()` and
// `Finish()` respectively. These should provide initialization and cleanup for
// static variables in the element class.
//
// `Initialize()` should be called before creating any `ElementBase<Derived>`
// elements. This means that elements must not be created as global or static
// variables. `Finish()` can be called after we are done with all
// `ElementBase<Derived>` elements.
template <typename Derived>
class ElementBase {
 public:
  // Call this before creating any `ElementBase<Derived>` elements.
  static void Initialize();
  // Call this after being done with `ElementBase<Derived>`.
  static void Finish();

  // Running this concurrently may lead to poor randomness in the height
  // distribution of skip list elements.
  ElementBase();
  // Uses random_int as a seed to generate a random height for the element.
  explicit ElementBase(size_t random_int);

  virtual ~ElementBase();
  ElementBase(const ElementBase&) = delete;
  ElementBase(ElementBase&&) = delete;
  ElementBase& operator=(const ElementBase&) = delete;
  ElementBase& operator=(ElementBase&&) = delete;

  Derived* GetPreviousElement() const;
  Derived* GetNextElement() const;

  // Returns a representative element from the list the element lives in. Two
  // elements have the same representative element if and only if they reside in
  // the same list.
  //
  // A representative element is only valid until the next `Join` or `Split`
  // call.
  Derived* FindRepresentative() const;

  // Concatenates the list that `left` lives in to the list that `right` lives
  // in. `left` must be the last element in its list. `right` must be the first
  // element in its list. `left` and `right` are allowed to be in the same list,
  // in which case joining them creates a cyclic list.
  //
  // May run concurrently with other `Join` calls so long as each `left` is
  // unique and each `right` is unique.
  static void Join(Derived* left, Derived* right);

  // Split the list right after this element (so this element will be the last
  // element in its list). Returns what was the successor to the element (the
  // first element on the right-hand side of the split). (If `Split` is called
  // multiple times concurrently on the same element, only one of the executions
  // will "succeed", and the others will return a null pointer.)
  //
  // May run concurrently with other `Split` calls.
  Derived* Split();

 protected:
  struct Neighbors { Derived* prev; Derived* next; };

  bool CASNext(int level, Derived* old_next, Derived* new_next);
  bool CASPrev(int level, Derived* old_prev, Derived* new_prev);
  // When called on element `v`, searches left starting from and including `v`
  // for the first element at the next level up.
  Derived* FindLeftParent(int level) const;
  // When called on element `v`, searches right starting from and including `v`
  // for the first element at the next level up.
  Derived* FindRightParent(int level) const;

  // We might think to make this an `ArrayAllocator<T>` instead of a
  // pointer to one, but then we run into a Static Initialization Order Fiasco.
  // When run, our program could choose to initialize `ArrayAllocator<T>`,
  // which uses `list_allocator<T>`, before `list_allocator<T>` is initialized.
  static concurrent_array_allocator::Allocator<Neighbors>* neighbor_allocator_;
  static pbbs::random default_randomness_;

  // neighbors_[i] holds neighbors at level i, where level 0 is the lowest level
  // and is the level at which the list contains all elements
  Neighbors* neighbors_;
  int height_;
};

///////////////////////////////////////////////////////////////////////////////
//                           Implementation below.                           //
///////////////////////////////////////////////////////////////////////////////

namespace _internal {

int GenerateHeight(size_t random_int);

}  // namespace _internal

template <typename Derived>
concurrent_array_allocator::Allocator<typename ElementBase<Derived>::Neighbors>*
    ElementBase<Derived>::neighbor_allocator_{nullptr};
template <typename Derived>
pbbs::random ElementBase<Derived>::default_randomness_{};

template <typename Derived>
void ElementBase<Derived>::Initialize() {
  if (neighbor_allocator_ == nullptr) {
    neighbor_allocator_ =
      new concurrent_array_allocator::Allocator<Neighbors>{};
  }
  Derived::DerivedInitialize();
}

template <typename Derived>
void ElementBase<Derived>::Finish() {
  if (neighbor_allocator_ != nullptr) {
    delete neighbor_allocator_;
  }
  Derived::DerivedFinish();
}

template <typename Derived>
ElementBase<Derived>::ElementBase() {
  size_t random_int{default_randomness_.rand()};
  default_randomness_ = default_randomness_.next();  // race if run concurrently
  height_ = _internal::GenerateHeight(random_int);
  neighbors_ = neighbor_allocator_->Allocate(height_);
  for (int i = 0; i < height_; i++) {
    neighbors_[i].prev = neighbors_[i].next = nullptr;
  }
}

template <typename Derived>
ElementBase<Derived>::ElementBase(size_t random_int) {
  height_ = _internal::GenerateHeight(random_int);
  neighbors_ = neighbor_allocator_->Allocate(height_);
  for (int i = 0; i < height_; i++) {
    neighbors_[i].prev = neighbors_[i].next = nullptr;
  }
}

template <typename Derived>
ElementBase<Derived>::~ElementBase() {
  neighbor_allocator_->Free(neighbors_, height_);
}

template <typename Derived>
bool ElementBase<Derived>::CASNext(
    int level, Derived* old_next, Derived* new_next) {
  return CAS(&neighbors_[level].next, old_next, new_next);
}

template <typename Derived>
bool ElementBase<Derived>::CASPrev(
    int level, Derived* old_prev, Derived* new_prev) {
  return CAS(&neighbors_[level].prev, old_prev, new_prev);
}

template <typename Derived>
Derived* ElementBase<Derived>::GetPreviousElement() const {
  return neighbors_[0].prev;
}

template <typename Derived>
Derived* ElementBase<Derived>::GetNextElement() const {
  return neighbors_[0].next;
}

template <typename Derived>
Derived* ElementBase<Derived>::FindLeftParent(int level) const {
  const Derived* current_element{static_cast<const Derived*>(this)};
  const Derived* start_element{current_element};
  do {
    if (current_element->height_ > level + 1) {
      return const_cast<Derived*>(current_element);
    }
    current_element = current_element->neighbors_[level].prev;
  } while (current_element != nullptr && current_element != start_element);
  return nullptr;
}

template <typename Derived>
Derived* ElementBase<Derived>::FindRightParent(int level) const {
  const Derived* current_element{static_cast<const Derived*>(this)};
  const Derived* start_element{current_element};
  do {
    if (current_element->height_ > level + 1) {
      return const_cast<Derived*>(current_element);
    }
    current_element = current_element->neighbors_[level].next;
  } while (current_element != nullptr && current_element != start_element);
  return nullptr;
}

template <typename Derived>
Derived* ElementBase<Derived>::FindRepresentative() const {
  // If the list is cyclic, return element on highest level, breaking ties in
  // favor of the lowest address.
  // If the list is not cyclic, then return the head element on the highest
  // level.

  const Derived* current_element{static_cast<const Derived*>(this)};
  const Derived* seen_element{nullptr};
  int current_level{current_element->height_ - 1};

  // walk up while moving forward
  while (current_element->neighbors_[current_level].next != nullptr &&
         seen_element != current_element) {
    if (seen_element == nullptr || current_element < seen_element) {
      seen_element = current_element;
    }
    current_element = current_element->neighbors_[current_level].next;
    const int top_level{current_element->height_ - 1};
    if (current_level < top_level) {
      current_level = top_level;
      seen_element = nullptr;
    }
  }

  if (seen_element == current_element) {  // list is a cycle
    return const_cast<Derived*>(seen_element);
  } else {
    // walk up while moving backward
    while (current_element->neighbors_[current_level].prev != nullptr) {
      current_element = current_element->neighbors_[current_level].prev;
      current_level = current_element->height_ - 1;
    }
    return const_cast<Derived*>(current_element);
  }
}

template <typename Derived>
void ElementBase<Derived>::Join(Derived* left, Derived* right) {
  int level{0};
  while (left != nullptr && right != nullptr) {
    if (left->neighbors_[level].next == nullptr &&
        left->CASNext(level, nullptr, right)) {
      // This CAS prevents read-write reordering of these `prev` pointers that
      // might cause concurrent `Join`s to collectively fail to find a link to
      // be added at a higher level.
      right->CASPrev(level, nullptr, left);
      left = left->FindLeftParent(level);
      right = right->FindRightParent(level);
      level++;
    } else {
      return;
    }
  }
}

template <typename Derived>
Derived* ElementBase<Derived>::Split() {
  // It's tempting to set `successor = GetNextElement()` here, but we need to
  // wait for the CAS in case multiple `Split` calls are made on the same
  // element.
  Derived* successor{nullptr};
  Derived* current_element{static_cast<Derived*>(this)};
  int level{0};
  while (current_element != nullptr) {
    Derived* next{current_element->neighbors_[level].next};
    if (next != nullptr && current_element->CASNext(level, next, nullptr)) {
      if (level == 0) {
        successor = next;
      }
      // Here there's an issue with read-write reordering like in `Join`, but
      // it's benign.  The read-write reorder in `Join` is an issue because it
      // means we might not find a path up to the next level when we should be
      // able to. On the other hand, this reordering might cause us to find a
      // path up to the next level when the path has already been cut. This
      // might cause a small amount of extra work, but it's not a correctness
      // issue.
      next->neighbors_[level].prev = nullptr;
      current_element = current_element->FindLeftParent(level);
      level++;
    } else {
      break;
    }
  }
  return successor;
}

}  // namespace parallel_skip_list
