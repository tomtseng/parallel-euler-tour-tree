#include <sequence/parallel_skip_list/include/augmented_skip_list.hpp>

#include <cassert>

#include <utilities/include/utils.h>

namespace parallel_skip_list {

using std::pair;

namespace {

  constexpr int NA{-1};
  concurrent_array_allocator::Allocator<int>* val_allocator;

  int* AllocateValueArray(int len) {
    int* values{val_allocator->Allocate(len)};
    for (int i = 0; i < len; i++) {
      values[i] = 1;
    }
    return values;
  }

}  // namespace

void AugmentedElement::DerivedInitialize() {
  if (val_allocator == nullptr) {
    val_allocator = new concurrent_array_allocator::Allocator<int>;
  }
}

void AugmentedElement::DerivedFinish() {
  if (val_allocator != nullptr) {
    delete val_allocator;
  }
}

AugmentedElement::AugmentedElement() :
  ElementBase<AugmentedElement>{}, update_level_{NA} {
  values_ = AllocateValueArray(height_);
}

AugmentedElement::AugmentedElement(size_t random_int) :
  ElementBase<AugmentedElement>{random_int}, update_level_{NA} {
  values_ = AllocateValueArray(height_);
}

AugmentedElement::~AugmentedElement() {
  val_allocator->Free(values_, height_);
}

void AugmentedElement::UpdateTopDownSequential(int level) {
  if (level == 0) {
    if (height_ == 1) {
      update_level_ = NA;
    }
    return;
  }

  if (update_level_ < level) {
    UpdateTopDownSequential(level - 1);
  }
  int sum{values_[level - 1]};
  AugmentedElement* curr{neighbors_[level - 1].next};
  while (curr != nullptr && curr->height_ < level + 1) {
    if (curr->update_level_ != NA && curr->update_level_ < level) {
      curr->UpdateTopDownSequential(level - 1);
    }
    sum += curr->values_[level - 1];
    curr = curr->neighbors_[level - 1].next;
  }
  values_[level] = sum;

  if (height_ == level + 1) {
    update_level_ = NA;
  }
}

// `v.UpdateTopDown(level)` updates the augmented values of descendants of `v`'s
// `level`-th node. `update_level_` is used to determine what nodes need
// updating. `update_level_` is reset to `NA` for all traversed nodes at end of
// this function.
void AugmentedElement::UpdateTopDown(int level) {
  if (level <= 6) {
    UpdateTopDownSequential(level);
    return;
  }

  // Recursively update augmented values of children.
  AugmentedElement* curr{this};
  do {
    if (curr->update_level_ != NA && curr->update_level_ < level) {
      cilk_spawn curr->UpdateTopDown(level - 1);
    }
    curr = curr->neighbors_[level - 1].next;
  } while (curr != nullptr && curr->height_ < level + 1);
  cilk_sync;

  // Now that children have correct augmented valeus, update self's augmented
  // value.
  int sum{values_[level - 1]};
  curr = neighbors_[level - 1].next;
  while (curr != nullptr && curr->height_ < level + 1) {
    sum += curr->values_[level - 1];
    curr = curr->neighbors_[level - 1].next;
  }
  values_[level] = sum;

  if (height_ == level + 1) {
    update_level_ = NA;
  }
}

// If `new_values` is non-null, for each `i`=0,1,...,`len`-1, assign value
// `new_vals[i]` to element `elements[i]`.
//
// If `new_values` is null, update the augmented values of the ancestors of
// `elements`, where the "ancestors" of element `v` refer to `v`,
// `v->FindLeftParent(0)`, `v->FindLeftParent(0)->FindLeftParent(1)`,
// `v->FindLeftParent(0)->FindLeftParent(2)`, and so on. This functionality is
// used privately to keep the augmented values correct when the list has
// structurally changed.
void AugmentedElement::BatchUpdate(
    AugmentedElement** elements, int* new_values, int len) {
  if (new_values != nullptr) {
    parallel_for (int i = 0; i < len; i++) {
      elements[i]->values_[0] = new_values[i];
    }
  }

  // The nodes whose augmented values need updating are the ancestors of
  // `elements`. Some nodes may share ancestors. `top_nodes` will contain,
  // without duplicates, the set of all ancestors of `elements` with no left
  // parents. From there we can walk down from those ancestors to update all
  // required augmented values.
  AugmentedElement** top_nodes{pbbs::new_array_no_init<AugmentedElement*>(len)};

  parallel_for (int i = 0; i < len; i++) {
    int level{0};
    AugmentedElement* curr{elements[i]};
    while (true) {
      int curr_update_level{curr->update_level_};
      if (curr_update_level == NA && CAS(&curr->update_level_, NA, level)) {
        level = curr->height_ - 1;
        AugmentedElement* parent{curr->FindLeftParent(level)};
        if (parent == nullptr) {
          top_nodes[i] = curr;
          break;
        } else {
          curr = parent;
          level++;
        }
      } else {
        // Someone other execution is shares this ancestor and has already
        // claimed it, so there's no need to walk further up.
        if (curr_update_level > level) {
          writeMin(&curr->update_level_, level);
        }
        top_nodes[i] = nullptr;
        break;
      }
    }
  }

  parallel_for (int i = 0; i < len; i++) {
    if (top_nodes[i] != nullptr) {
      top_nodes[i]->UpdateTopDown(top_nodes[i]->height_ - 1);
    }
  }

  pbbs::delete_array(top_nodes, len);
}

void AugmentedElement::BatchJoin(
    pair<AugmentedElement*, AugmentedElement*>* joins, int len) {
  AugmentedElement** join_lefts{
      pbbs::new_array_no_init<AugmentedElement*>(len)};
  parallel_for (int i = 0; i < len; i++) {
    Join(joins[i].first, joins[i].second);
    join_lefts[i] = joins[i].first;
  }
  BatchUpdate(join_lefts, nullptr, len);
  pbbs::delete_array(join_lefts, len);
}

void AugmentedElement::BatchSplit(AugmentedElement** splits, int len) {
  parallel_for (int i = 0; i < len; i++) {
    splits[i]->Split();
  }
  parallel_for (int i = 0; i < len; i++) {
    AugmentedElement* curr{splits[i]};
    // `can_proceed` breaks ties when there are duplicate splits. When two
    // splits occur at the same place, only one of them should walk up and
    // update.
    bool can_proceed{
        curr->update_level_ == NA && CAS(&curr->update_level_, NA, 0)};
    if (can_proceed) {
      // Update values of `curr`'s ancestors.
      int sum{curr->values_[0]};
      int level{0};
      while (true) {
        if (level < curr->height_ - 1) {
          level++;
          curr->values_[level] = sum;
        } else {
          curr = curr->neighbors_[level].prev;
          if (curr == nullptr) {
            break;
          } else {
            sum += curr->values_[level];
          }
        }
      }
    }
  }
  parallel_for (int i = 0; i < len; i++) {
    splits[i]->update_level_ = NA;
  }
}

int AugmentedElement::GetSubsequenceSum(
    const AugmentedElement* left, const AugmentedElement* right) {
  int level{0};
  int sum{right->values_[level]};
  while (left != right) {
    level = min(left->height_, right->height_) - 1;
    if (level == left->height_ - 1) {
      sum += left->values_[level];
      left = left->neighbors_[level].next;
    } else {
      right = right->neighbors_[level].prev;
      sum += right->values_[level];
    }
  }
  return sum;
}

int AugmentedElement::GetSum() const {
  // Here we use knowledge of the implementation of `FindRepresentative()`.
  // `FindRepresentative()` gives some element that reaches the top level of the
  // list. For acyclic lists, the element is the leftmost one.
  AugmentedElement* root{FindRepresentative()};
  // Sum the values across the top level of the list.
  int level{root->height_ - 1};
  int sum{root->values_[level]};
  AugmentedElement* curr{root->neighbors_[level].next};
  while (curr != nullptr && curr != root) {
    sum += curr->values_[level];
    curr = curr->neighbors_[level].next;
  }
  if (curr == nullptr) {
    // The list is not circular, so we need to traverse backwards to beginning
    // of list and sum values to the left of `root`.
    curr = root;
    while (true) {
      while (level >= 0 && curr->neighbors_[level].prev == nullptr) {
        level--;
      }
      if (level < 0) {
        break;
      }
      while (curr->neighbors_[level].prev != nullptr) {
        curr = curr->neighbors_[level].prev;
        sum += curr->values_[level];
      }
    }
  }
  return sum;
}

}  // namespace parallel_skip_list
