// Some structural details:
// To represent a tree in a forest, for each vertex v, add a loop edge (v, v),
// and for each edge {u, v}, replace it with two directed edges (u, v) and (v,
// u). Because the transformed tree has equal indegree and outdegree for each
// vertex, it admits an Euler tour.
//
// Take any Euler tour of the tree and place it in a circular sequence data
// structure like a skip list. Euler tours behave nicely under edge additions
// and edge deletions, so links and cuts reduce to a few splits and joins on
// sequences.
#include <dynamic_trees/parallel_euler_tour_tree/include/euler_tour_tree.hpp>

#include <utility>

#include <sequence/parallel_skip_list/include/skip_list_base.hpp>
#include <utilities/include/blockRadixSort.h>
#include <utilities/include/list_allocator.h>
#include <utilities/include/seq.h>
#include <utilities/include/sequence_ops.h>
#include <utilities/include/utils.h>

namespace parallel_euler_tour_tree {

using Element = _internal::Element;
using std::pair;

namespace {

  // On BatchCut, randomly ignore 1/`kBatchCutRecursiveFactor` cuts and recurse
  // on them later.
  constexpr int kBatchCutRecursiveFactor{100};

  // Note: we never call `finish()` on this.
  list_allocator<_internal::Element> allocator{};

  void BatchCutSequential(EulerTourTree* ett, pair<int, int>* cuts, int len) {
    for (int i = 0; i < len; i++) {
      ett->Cut(cuts[i].first, cuts[i].second);
    }
  }

  void BatchLinkSequential(EulerTourTree* ett, pair<int, int>* links, int len) {
    for (int i = 0; i < len; i++) {
      ett->Link(links[i].first, links[i].second);
    }
  }

}  // namespace

EulerTourTree::EulerTourTree(int num_vertices)
    : num_vertices_{num_vertices} , edges_{num_vertices_} , randomness_{} {
  allocator.init();
  Element::Initialize();
  vertices_ = pbbs::new_array_no_init<Element>(num_vertices_);
  parallel_for (int i = 0; i < num_vertices_; i++) {
    new (&vertices_[i]) Element{randomness_.ith_rand(i)};
    // The Euler tour on a vertex v (a singleton tree) is simply (v, v).
    Element::Join(&vertices_[i], &vertices_[i]);
  }
  randomness_ = randomness_.next();
}

EulerTourTree::~EulerTourTree() {
  pbbs::delete_array(vertices_, num_vertices_);
  edges_.FreeElements(&allocator);
  Element::Finish();
}

bool EulerTourTree::IsConnected(int u, int v) const {
  return vertices_[u].FindRepresentative() == vertices_[v].FindRepresentative();
}

void EulerTourTree::Link(int u, int v) {
  Element* uv{allocator.alloc()};
  new (uv) Element{randomness_.ith_rand(0)};
  Element* vu = allocator.alloc();
  new (vu) Element{randomness_.ith_rand(1)};
  randomness_ = randomness_.next();
  uv->twin_ = vu;
  vu->twin_ = uv;
  edges_.Insert(u, v, uv);
  Element* u_left{&vertices_[u]};
  Element* v_left{&vertices_[v]};
  Element* u_right{u_left->Split()};
  Element* v_right{v_left->Split()};
  Element::Join(u_left, uv);
  Element::Join(uv, v_right);
  Element::Join(v_left, vu);
  Element::Join(vu, u_right);
}

void EulerTourTree::BatchLink(pair<int, int>* links, int len) {
  if (len <= 75) {
    BatchLinkSequential(this, links, len);
    return;
  }

  // For each added edge {x, y}, allocate elements (x, y) and (y, x).
  // For each vertex x that shows up in an added edge, split on (x, x). Let
  // succ(x) denote the successor of (x, x) prior to splitting.
  // For each vertex x, identify which y_1, y_2, ... y_k that x will be newly
  // connected to by performing a semisort on {(x, y), (y, x) : {x, y} is an
  // added edge}.
  // If x has new neighbors y_1, y_2, ..., y_k, join (x, x) to (x, y_1). Join
  // (y_i,x) to (x, y_{i+1}) for each i < k. Join (y_k, x) to succ(x).

  pair<int, int>* links_both_dirs{
      pbbs::new_array_no_init<pair<int, int>>(2 * len)};
  parallel_for (int i = 0; i < len; i++) {
    links_both_dirs[2 * i] = links[i];
    links_both_dirs[2 * i + 1] = make_pair(links[i].second, links[i].first);
  }
  intSort::iSort(
      links_both_dirs, 2 * len, num_vertices_ + 1, firstF<int, int>());

  Element** split_successors{pbbs::new_array_no_init<Element*>(2 * len)};
  parallel_for (uint64_t i = 0; i < 2 * len; i++) {
    int u, v;
    std::tie(u, v) = links_both_dirs[i];

    // split on each vertex that appears in the input
    if (i == 2 * len - 1 || u != links_both_dirs[i + 1].first) {
      split_successors[i] = vertices_[u].Split();
    }

    // allocate edge element
    if (u < v) {
      Element* uv{allocator.alloc()};
      new (uv) Element{randomness_.ith_rand(2 * i)};
      Element* vu{allocator.alloc()};
      new (vu) Element{randomness_.ith_rand(2 * i + 1)};
      uv->twin_ = vu;
      vu->twin_ = uv;
      edges_.Insert(u, v, uv);
    }
  }
  randomness_ = randomness_.next();

  parallel_for (int i = 0; i < 2 * len; i++) {
    int u, v;
    std::tie(u, v) = links_both_dirs[i];
    Element* uv{edges_.Find(u, v)};
    Element* vu{uv->twin_};
    if (i == 0 ||
        u != links_both_dirs[i - 1].first) {
      Element::Join(&vertices_[u], uv);
    }
    if (i == 2 * len - 1 ||
        u != links_both_dirs[i + 1].first) {
      Element::Join(vu, split_successors[i]);
    } else {
      int u2, v2;
      std::tie(u2, v2) = links_both_dirs[i + 1];
      Element::Join(vu, edges_.Find(u2, v2));
    }
  }

  pbbs::delete_array(links_both_dirs, 2 * len);
  pbbs::delete_array(split_successors, 2 * len);
}

void EulerTourTree::Cut(int u, int v) {
  Element* uv{edges_.Find(u, v)};
  Element* vu{uv->twin_};
  edges_.Delete(u, v);
  Element* u_left{uv->GetPreviousElement()};
  Element* v_left{vu->GetPreviousElement()};
  Element* v_right{uv->Split()};
  Element* u_right{vu->Split()};
  u_left->Split();
  v_left->Split();
  uv->~Element();
  allocator.free(uv);
  vu->~Element();
  allocator.free(vu);
  Element::Join(u_left, u_right);
  Element::Join(v_left, v_right);
}

// `ignored`, `join_targets`, and `edge_elements` are scratch space.
// `ignored[i]` will be set to true if `cuts[i]` will not be executed in this
// round of recursion.
// `join_targets` stores sequence elements that need to be joined to each other.
// `edge_elements[i]` stores a pointer to the sequence element corresponding to
// edge `cuts[i]`.
void EulerTourTree::BatchCutRecurse(pair<int, int>* cuts, int len,
    bool* ignored, Element** join_targets, Element** edge_elements) {
  if (len <= 75) {
    BatchCutSequential(this, cuts, len);
    return;
  }

  // Notation: "(x, y).next" is the next element in the tour (x, y) is in. "(x,
  // y).prev" is the previous element. "(x, y).twin" is (y, x).
  // For each edge {x, y} to cut:
  // Sequentially, we'd want to join (y, x).prev to (x, y).next and (x, y).prev
  // to (y, x).next. We can't correctly do this if any of those four elements
  // are to be cut and removed as well. Instead, for dealing with connecting (y,
  // x).prev to (x, y).next (dealing with connecting (x, y).prev to (y,x ).next
  // is symmetric), we do the following:
  // - If (y, x).prev is to be cut, then do nothing --- some other thread will
  // deal with this.
  // - Otherwise, start with element e = (x, y).next. So long as e is to be cut,
  // traverse to the next possible join location at e := e.next.twin. Join
  // (y,x).prev to e.
  // This strategy doesn't have good depth since we may have to traverse on e
  // for a long time. To fix this, we randomly ignore some cuts so that all
  // traversal lengths are O(log n) with high probability. We perform all
  // unignored cuts as described above, and recurse on the ignored cuts
  // afterwards.

  parallel_for (int i = 0; i < len; i++) {
    ignored[i] = randomness_.ith_rand(i) % kBatchCutRecursiveFactor == 0;

    if (!ignored[i]) {
      int u, v;
      std::tie(u, v) = cuts[i];
      Element* uv{edges_.Find(u, v)};
      edge_elements[i] = uv;
      Element* vu{uv->twin_};
      uv->split_mark_ = vu->split_mark_ = true;
    }
  }
  randomness_ = randomness_.next();

  parallel_for (int i = 0; i < len; i++) {
    if (!ignored[i]) {
      Element* uv{edge_elements[i]};
      Element* vu{uv->twin_};

      Element* left_target{uv->GetPreviousElement()};
      if (left_target->split_mark_) {
        join_targets[4 * i] = nullptr;
      } else {
        Element* right_target{vu->GetNextElement()};
        while (right_target->split_mark_) {
          right_target = right_target->twin_->GetNextElement();
        }
        join_targets[4 * i] = left_target;
        join_targets[4 * i + 1] = right_target;
      }

      left_target = vu->GetPreviousElement();
      if (left_target->split_mark_) {
        join_targets[4 * i + 2] = nullptr;
      } else {
        Element* right_target{uv->GetNextElement()};
        while (right_target->split_mark_) {
          right_target = right_target->twin_->GetNextElement();
        }
        join_targets[4 * i + 2] = left_target;
        join_targets[4 * i + 3] = right_target;
      }
    }
  }

  parallel_for (int i = 0; i < len; i++) {
    if (!ignored[i]) {
      Element* uv{edge_elements[i]};
      Element* vu{uv->twin_};
      uv->Split();
      vu->Split();
      Element* predecessor{uv->GetPreviousElement()};
      if (predecessor != nullptr) {
        predecessor->Split();
      }
      predecessor = vu->GetPreviousElement();
      if (predecessor != nullptr) {
        predecessor->Split();
      }
    }
  }

  parallel_for (int i = 0; i < len; i++)  {
    if (!ignored[i]) {
      // Here we must use `edge_elements[i]` instead of `edges_.Find(u, v)`
      // because the concurrent hash table cannot handle simultaneous lookups
      // and deletions.
      Element* uv{edge_elements[i]};
      Element* vu{uv->twin_};
      uv->~Element();
      allocator.free(uv);
      vu->~Element();
      allocator.free(vu);
      int u, v;
      std::tie(u, v) = cuts[i];
      edges_.Delete(u, v);

      if (join_targets[4 * i] != nullptr) {
        Element::Join(join_targets[4 * i], join_targets[4 * i + 1]);
      }
      if (join_targets[4 * i + 2] != nullptr) {
        Element::Join(join_targets[4 * i + 2], join_targets[4 * i + 3]);
      }
    }
  }

  seq::sequence<pair<int, int>> cuts_seq{
      seq::sequence<pair<int, int>>(cuts, len)};
  seq::sequence<bool> ignored_seq{seq::sequence<bool>(ignored, len)};
  seq::sequence<pair<int, int>> next_cuts_seq{
    pbbs::pack(cuts_seq, ignored_seq)};
  BatchCutRecurse(next_cuts_seq.as_array(), next_cuts_seq.size(),
      ignored, join_targets, edge_elements);
  pbbs::delete_array(next_cuts_seq.as_array(), next_cuts_seq.size());
}

void EulerTourTree::BatchCut(pair<int, int>* cuts, int len) {
  if (len <= 75) {
    BatchCutSequential(this, cuts, len);
    return;
  }
  bool* ignored{pbbs::new_array_no_init<bool>(len)};
  Element** join_targets{pbbs::new_array_no_init<Element*>(4 * len)};
  Element** edge_elements{pbbs::new_array_no_init<Element*>(len)};
  BatchCutRecurse(cuts, len, ignored, join_targets, edge_elements);
  pbbs::delete_array(edge_elements, len);
  pbbs::delete_array(join_targets, 4 * len);
  pbbs::delete_array(ignored, len);
}

}  // namespace parallel_euler_tour_tree
