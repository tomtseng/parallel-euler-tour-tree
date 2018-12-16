#include <sequence/parallel_treap/include/treap.hpp>

#include <cassert>

#include <utilities/include/debug.hpp>
#include <utilities/include/random.h>
#include <utilities/include/utils.h>

using Node = treap::Node;

const int num_nodes = 10000;
Node* nodes;

bool split_points[num_nodes];
int start_index_of_list[num_nodes];

void prime_sieve() {
  split_points[2] = true;
  for (int i = 3; i < num_nodes; i += 2)
    split_points[i] = true;
  for (long long i = 3; i * i < num_nodes; i += 2) {
    if (split_points[i]) {
      for (long long j = i * i; j < num_nodes; j += 2 * i) {
        split_points[j] = false;
      }
    }
  }
}

int main() {
  pbbs::random r;
  nodes = pbbs::new_array_no_init<Node>(num_nodes);
  parallel_for (int i = 0; i < num_nodes; i++) {
    new (&nodes[i]) Node(r.ith_rand(i));
  }

  prime_sieve();

  int start_index = 0;
  for (int i = 0; i < num_nodes; i++) {
    start_index_of_list[i] = start_index;
    if (split_points[i]) {
      start_index = i + 1;
    }
  }

  for (int i = 0; i < num_nodes; i++) {
    Node* iroot = nodes[i].GetRoot();
    for (int j = i + 1; j < num_nodes; j++) {
      assert(iroot != nodes[j].GetRoot());
    }
  }

  std::cout << "*** TEST SEQUENTIAL ***" << std::endl;
  for (int T = 0; T < 3; T++) {
    for (int i = 0; i < num_nodes - 1; i++) {
      Node::Join(&nodes[i], &nodes[i + 1]);
    }

    Node* root0 = nodes[0].GetRoot();
    for (int i = 0; i < num_nodes; i++) {
      assert(root0 == nodes[i].GetRoot());
    }

    for (int i = 0; i < num_nodes; i++) {
      if (split_points[i]) {
        nodes[i].Split();
      }
    }

    for (int i = 0; i < num_nodes; i++) {
      const int start = start_index_of_list[i];
      assert(nodes[start].GetRoot() == nodes[i].GetRoot());
      if (start > 0) {
        assert(nodes[start - 1].GetRoot() != nodes[i].GetRoot());
      }
    }

    for (int i = 0; i < num_nodes - 1; i++) {
      if (split_points[i]) {
        Node::Join(&nodes[i], &nodes[(i + 1)]);
      }
    }

    root0 = nodes[0].GetRoot();
    for (int i = 0; i < num_nodes; i++) {
      assert(root0 == nodes[i].GetRoot());
    }

    for (int i = 0; i < num_nodes - 1; i++) {
      nodes[i].Split();
    }

    for (int i = 0; i < num_nodes; i++) {
      Node* iroot = nodes[i].GetRoot();
      for (int j = i + 1; j < num_nodes; j++) {
        assert(iroot != nodes[j].GetRoot());
      }
    }
  }

  std::pair<Node*, Node*>* joins =
    pbbs::new_array_no_init<std::pair<Node*, Node*>>(num_nodes);
  Node** splits = pbbs::new_array_no_init<Node*>(num_nodes);
  std::cout << "*** TEST PARALLEL ***" << std::endl;
  for (int T = 0; T < 3; T++) {
    parallel_for (int i = 0; i < num_nodes - 1; i++) {
      joins[i] = std::make_pair(&nodes[i], &nodes[i + 1]);
    }
    Node::BatchJoin(joins, num_nodes - 1);

    Node* root0 = nodes[0].GetRoot();
    parallel_for (int i = 0; i < num_nodes; i++) {
      assert(root0 == nodes[i].GetRoot());
    }

    int len = 0;
    for (int i = 0; i < num_nodes; i++) {
      if (split_points[i]) {
        splits[len++] = &nodes[i];
      }
    }
    Node::BatchSplit(splits, len);

    parallel_for (int i = 0; i < num_nodes; i++) {
      const int start = start_index_of_list[i];
      assert(nodes[start].GetRoot() == nodes[i].GetRoot());
      if (start > 0) {
        assert(nodes[start - 1].GetRoot() != nodes[i].GetRoot());
      }
    }

    len = 0;
    for (int i = 0; i < num_nodes; i++) {
      if (split_points[i]) {
        joins[len++] = std::make_pair(&nodes[i], &nodes[i + 1]);
      }
    }
    Node::BatchJoin(joins, len);

    root0 = nodes[0].GetRoot();
    parallel_for (int i = 0; i < num_nodes; i++) {
      assert(root0 == nodes[i].GetRoot());
    }

    parallel_for (int i = 0; i < num_nodes - 1; i++) {
      splits[i] = &nodes[i];
    }
    Node::BatchSplit(splits, num_nodes - 1);

    parallel_for (int i = 0; i < num_nodes; i++) {
      Node* iroot = nodes[i].GetRoot();
      for (int j = i + 1; j < num_nodes; j++) {
        assert(iroot != nodes[j].GetRoot());
      }
    }
  }

  std::cout << "*** TEST PARALLEL MORE ***" << std::endl;
  parallel_for (int i = 0; i < num_nodes - 1; i++) {
    joins[i] = std::make_pair(&nodes[i], &nodes[i + 1]);
  }
  for (int T = 0; T < 3; T++) {
    splits[0] = &nodes[num_nodes / 5];
    splits[1] = &nodes[num_nodes / 5 * 2];
    splits[2] = &nodes[num_nodes / 5 * 3];
    splits[3] = &nodes[num_nodes / 5 * 4];
    Node::BatchSplit(splits, 4);

    parallel_for (int i = 0; i < num_nodes - 1; i++) {
      splits[i] = &nodes[i];
    }
    Node::BatchSplit(splits, num_nodes - 1);

    parallel_for (int i = 0; i < num_nodes; i++) {
      Node* iroot = nodes[i].GetRoot();
      for (int j = i + 1; j < num_nodes; j++) {
        assert(iroot != nodes[j].GetRoot());
      }
    }

    Node::BatchJoin(joins, num_nodes - 1);

    Node* root0 = nodes[0].GetRoot();
    parallel_for (int i = 0; i < num_nodes; i++) {
      assert(root0 == nodes[i].GetRoot());
    }
  }

  pbbs::delete_array(splits, num_nodes);
  pbbs::delete_array(joins, num_nodes);
  pbbs::delete_array(nodes, num_nodes);

  std::cout << "Test complete." << std::endl;

  return 0;
}
