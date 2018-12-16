#include <sequence/parallel_treap/include/treap.hpp>

#include <tuple>

#include <utilities/include/blockRadixSort.h>
#include <utilities/include/random.h>

namespace treap {

using std::pair;

namespace {

  pbbs::random default_randomness;
  const int kSplitSequentialThreshold = 256;
  const int kSplitOneTreeSequentialThreshold = 64;
  const int kJoinSequentialThreshold = 64;
  // On BatchJoin, randomly ignore 1/`kBatchJoinRecursiveFactor` cuts and
  // recurse on them later.
  const int kBatchJoinRecursiveFactor = 20;

}  // namespace

Node::Node(unsigned random_int)
  : parent_(nullptr)
  , child_{nullptr, nullptr}
  , priority_(random_int)
  , right_joiner_(nullptr)
  , has_left_joiner_(false) {}

Node::Node() : Node(default_randomness.rand()) {
  default_randomness = default_randomness.next();  // race
}

void Node::AssignChild(int i, Node* v) {
  if (v != nullptr) {
    v->parent_ = this;
  }
  child_[i] = v;
}

Node* Node::GetRoot() const {
  const Node* current = this;
  while (current->parent_ != nullptr) {
    current = current->parent_;
  }
  return const_cast<Node*>(current);
}

pair<Node*, Node*> Node::Split() {
  Node* lesser = nullptr;
  Node* greater = child_[1];
  if (child_[1] != nullptr) {
    child_[1]->parent_ = nullptr;
    AssignChild(1, nullptr);
  }

  Node* current = this;
  bool traversed_up_from_right = 1;
  bool next_direction;
  while (current != nullptr) {
    Node* p = current->parent_;
    if (p != nullptr) {
      next_direction = p->child_[1] == current;
      p->AssignChild(next_direction, nullptr);
      current->parent_ = nullptr;
    }
    if (traversed_up_from_right) {
      lesser = Join(current, lesser);
    } else {
      greater = Join(greater, current);
    }

    traversed_up_from_right = next_direction;
    current = p;
  }
  return {lesser, greater};
}

Node* Node::JoinRoots(Node* lesser, Node* greater) {
  if (lesser == nullptr) {
    return greater;
  } else if (greater == nullptr) {
    return lesser;
  }

  if (lesser->priority_ > greater->priority_) {
    lesser->AssignChild(1, JoinRoots(lesser->child_[1], greater));
    return lesser;
  } else {
    greater->AssignChild(0, JoinRoots(lesser, greater->child_[0]));
    return greater;
  }
}

Node* Node::Join(Node* lesser, Node* greater) {
  return JoinRoots(
      lesser == nullptr ? nullptr : lesser->GetRoot(),
      greater == nullptr ? nullptr : greater->GetRoot());
}

// Implementation: Divide and conquer --- Choose a random split and execute it.
// Separate the remaining splits based on which tree they operate on and recurse
// in parallel.
void BatchSplitOneTree(Node** splits, int len, pbbs::random randomness) {
  if (len < kSplitOneTreeSequentialThreshold) {
    for (int i = 0; i < len; i++) {
      splits[i]->Split();
    }
    return;
  }

  int pivot_index = randomness.rand() % len;
  Node* pivot_node = splits[pivot_index];
  Node* left_parent, * right_parent;
  std::tie(left_parent, right_parent) = pivot_node->Split();

  bool* flags = pbbs::new_array_no_init<bool>(len);
  parallel_for (int i = 0; i < len; i++) {
    flags[i] = splits[i]->GetRoot() == right_parent;
  }
  Node** splits_right = pbbs::new_array_no_init<Node*>(len);
  const int len_right = utils::sequence::pack(splits, splits_right, flags, len);
  cilk_spawn BatchSplitOneTree(splits_right, len_right, randomness.fork(1));

  parallel_for (int i = 0; i < len; i++) {
    flags[i] = !flags[i];
  }
  flags[pivot_index] = 0;
  Node** splits_left = pbbs::new_array_no_init<Node*>(len);
  const int len_left = utils::sequence::pack(splits, splits_left, flags, len);
  BatchSplitOneTree(splits_left, len_left, randomness.fork(2));

  pbbs::delete_array(splits_left, len_left);
  cilk_sync;
  pbbs::delete_array(splits_right, len_right);
}

// O(k log n log k) expected work and O(log n log k) depth with high probability
// for k splits over n elements.
void Node::BatchSplit(Node** splits, int len) {
  if (len < kSplitSequentialThreshold) {
    for (int i = 0; i < len; i++) {
      splits[i]->Split();
    }
    return;
  }

  // Implementation: Sort splits to find sets of splits that all operate on same
  // tree. Call BatchSplitOneTree on each set.

  // Sort splits to find splits that all operate on same tree.
  pair<uintptr_t, Node*>* splits_by_tree =
    pbbs::new_array_no_init<pair<uintptr_t, Node*>>(len);
  parallel_for (int i = 0; i < len; i++) {
    splits_by_tree[i] = make_pair(
        reinterpret_cast<uintptr_t>(splits[i]->GetRoot()), splits[i]);
  }
  intSort::iSort(splits_by_tree, len,
    utils::sequence::mapReduce<uintptr_t>(
      splits_by_tree,
      len,
      maxF<uintptr_t>(),
      firstF<uintptr_t, Node*>()) + 1,
    firstF<uintptr_t, Node*>());

  parallel_for (int i = 0; i < len; i++) {
    // In parallel, split on each tree
    if (i == 0 || splits_by_tree[i].first != splits_by_tree[i - 1].first) {
      // Left endpoint of a contiguous batch of splits on a particular tree.
      // Binary search for right endpoint.
      int lo = i + 1;
      int hi = len;
      const uintptr_t this_tree = splits_by_tree[i].first;
      while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (splits_by_tree[mid].first == this_tree) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      const int right_endpoint = lo;
      const int len_this_tree = right_endpoint - i;

      if (len_this_tree < kSplitOneTreeSequentialThreshold) {
        for (int j = i; j < right_endpoint; j++) {
          splits_by_tree[j].second->Split();
        }
      } else {
        Node** splits_on_this_tree =
           pbbs::new_array_no_init<Node*>(len_this_tree);
        parallel_for (int j = i; j < right_endpoint; j++) {
          splits_on_this_tree[j - i] = splits_by_tree[j].second;
        }
        BatchSplitOneTree(
            splits_on_this_tree, len_this_tree, default_randomness.fork(i));
        pbbs::delete_array(splits_on_this_tree, len_this_tree);
      }
    }
  }
  default_randomness = default_randomness.next();

  pbbs::delete_array(splits_by_tree, len);
}

void Node::BatchJoinRecurse(
    pair<Node*, Node*>* joins,
    int len,
    bool* ignored,
    Node** left_roots) {
  if (len < kJoinSequentialThreshold) {
    for (int i = 0; i < len; i++) {
      Join(joins[i].first, joins[i].second);
    }
    return;
  }

  // Implementation: Ignore a fraction of the joins. The remaining joins induce
  // a linked list on the trees where each list is not too long. In parallel on
  // each list, walk sequentially from left-to-right and perform joins.

  parallel_for (int i = 0; i < len; i++) {
    ignored[i] =
      default_randomness.ith_rand(i) % kBatchJoinRecursiveFactor == 0;
  }
  default_randomness = default_randomness.next();

  parallel_for (int i = 0; i < len; i++) {
    if (!ignored[i]) {
      Node* left_root = joins[i].first->GetRoot();
      Node* right_root = joins[i].second->GetRoot();
      left_root->right_joiner_ = right_root;
      right_root->has_left_joiner_ = true;
      left_roots[i] = left_root;
    }
  }

  parallel_for (int i = 0; i < len; i++) {
    if (!ignored[i] && !left_roots[i]->has_left_joiner_) {
      Node* current = left_roots[i];
      Node* next = current->right_joiner_;
      current->right_joiner_ = nullptr;
      while (next != nullptr) {
        Node* next_next = next->right_joiner_;
        next->right_joiner_ = nullptr;
        next->has_left_joiner_ = false;
        current = JoinRoots(current, next);
        next = next_next;
      }
    }
  }

  pair<Node*, Node*>* next_joins =
    pbbs::new_array_no_init<pair<Node*, Node*>>(len);
  int next_joins_len = utils::sequence::pack(joins, next_joins, ignored, len);
  BatchJoinRecurse(next_joins, next_joins_len, ignored, left_roots);
  pbbs::delete_array(next_joins, len);
}

// O(k log n) expected work and O(log n log k) depth with high probability for k
// joins over n elements.
void Node::BatchJoin(pair<Node*, Node*>* joins, int len) {
  if (len < kJoinSequentialThreshold) {
    for (int i = 0; i < len; i++) {
      Join(joins[i].first, joins[i].second);
    }
    return;
  }

  bool* ignored = pbbs::new_array_no_init<bool>(len);
  Node** left_roots = pbbs::new_array_no_init<Node*>(len);
  BatchJoinRecurse(joins, len, ignored, left_roots);
  pbbs::delete_array(left_roots, len);
  pbbs::delete_array(ignored, len);
}

}  // namespace treap
