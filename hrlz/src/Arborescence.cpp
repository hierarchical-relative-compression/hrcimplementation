#include<hrlz/Arborescence.hpp>
#include<vector>
#include<map>
#include<set>
#include<cstdint>
#include<algorithm>
#include<iostream>
#include<ext/pb_ds/priority_queue.hpp>
#include<hrlz/UnionFind.hpp>
#include<cassert>
#include<hrlz/PairingHeap.hpp>
#include<iostream>

const std::size_t END = SIZE_MAX;

struct WeightedEdgeOffsetter {
  WeightedEdge operator()(const WeightedEdge& e, const int64_t& offset) {
    assert(offset >= 0 || e.weight >= (std::size_t) -offset);
    return WeightedEdge(e.from, e.to, e.weight + offset);
  }
};

struct TwoLevelHeap;

struct TwoLevelHeap {
public:
  TwoLevelHeap(): _offset(), _top(), _bottom() {}
  TwoLevelHeap(TwoLevelHeap&& moved) = default;
  TwoLevelHeap(std::vector<WeightedEdge>&& q): _offset(0), _top(), _bottom() {
    _top.emplace(q.back(), 0);
    _bottom.emplace_back(std::move(q), 0);
  }
  TwoLevelHeap& operator=(TwoLevelHeap&& moved) = default;
  WeightedEdge top() {
    return WeightedEdgeOffsetter()(_top.top().first, _offset);
  }
  bool empty() {
    return _top.empty();
  }
  void pop() {
    auto from = _top.top().second;
    _top.pop();
    _bottom[from].first.pop_back();
    if(!_bottom[from].first.empty()) {
      _top.emplace(WeightedEdgeOffsetter()(_bottom[from].first.back(), _bottom[from].second), from);
    }
  }

  void offset(int64_t offset) {
    _offset += offset;
  }

  void join(TwoLevelHeap&& other) {
    if(_top.size() < other._top.size()) {
      std::swap(*this, other);
    }
    for(auto& bottom: other._bottom) {
      if(bottom.first.empty()) {
        continue;
      }
      auto back = bottom.first.back();
      auto offset = bottom.second + other._offset - _offset;
      _top.emplace(WeightedEdgeOffsetter()(back, offset), _bottom.size());
      _bottom.emplace_back(std::move(bottom.first), offset);
    }
  }
private:
  int64_t _offset;
  std::priority_queue<std::pair<WeightedEdge, std::size_t>, std::vector<std::pair<WeightedEdge, std::size_t>>, std::greater<std::pair<WeightedEdge, std::size_t>>> _top;
  std::vector<std::pair<std::vector<WeightedEdge>, int64_t>> _bottom;
};

typedef PairingHeap<WeightedEdge, std::less<WeightedEdge>, int64_t, WeightedEdgeOffsetter> PHeap;

struct ForestNode {
  ForestNode(Edge e): created_from(e), parent(END), children() {}
  Edge created_from;
  std::size_t parent;
  std::vector<std::size_t> children; 
};

struct ForestResult {
  std::set<std::size_t> roots;
  std::vector<ForestNode> nodes;
  std::map<Edge, std::size_t> edge_to_node;
  DynArray<std::size_t> representative;
  ForestResult(std::size_t n): representative(n) {
    for(std::size_t i = 0; i < n; ++i) {
      representative[i] = END;
    }
  }
  void delete_representative_to_root_path(std::size_t repr) {
    auto from_child = END; 
    auto leaf = representative[repr];
    representative[repr] = END;
    while(leaf != END) {
      for(auto child: nodes[leaf].children) {
        if(from_child != child) {
          roots.insert(child);
        }
        nodes[child].parent = END;
      }
      if(nodes[leaf].parent == END) {
        roots.erase(leaf);
      }
      from_child = leaf;
      leaf = nodes[leaf].parent;
    }
  }
  void add_child(std::size_t node, std::size_t child) {
    assert(node != END);
    assert(child != END);
    assert(nodes[child].parent == END);
    assert(node != child);
    nodes[child].parent = node;
    roots.erase(child);
    nodes[node].children.push_back(child);
  }
  std::size_t add_node(Edge e) {
    auto index = nodes.size();
    nodes.emplace_back(e);
    edge_to_node[e] = index;
    roots.insert(index);
    if(representative[e.to] == END) {
      representative[e.to] = index;
    }
    return index;
  }
};


struct Cycle {
  std::vector<WeightedEdge> edges;
  std::vector<std::size_t> edge_nums;
  std::vector<std::size_t> components;
  WeightedEdge heavy;
};

// Traverses the cycle edge is a part of. Follows entering edge of each strongly connected component
WeightedEdge max_edge_in_cycle(const DynArray<WeightedEdge>& enter, UnionFindNoWeight& strong, WeightedEdge edge) {
  auto heavy = edge;
  while(edge.from != END) {
    if(heavy.weight < edge.weight) {
      heavy = edge;
    }
    edge = enter[strong.find(edge.from)];
  }
  return heavy;
}

Cycle strong_path(const DynArray<WeightedEdge>& enter, const DynArray<std::size_t>& enter_num, UnionFindNoWeight& strong, WeightedEdge edge, std::size_t edge_num) {
  Cycle result({edge}, {edge_num}, {strong.find(edge.to)}, edge);
  edge = enter[strong.find(edge.from)];
  while (edge.from != END) {
    result.edges.push_back(edge);
    result.components.push_back(strong.find(edge.to));
    result.edge_nums.push_back(enter_num[strong.find(edge.to)]);
    if(result.heavy.weight < edge.weight) {
      result.heavy = edge;
    }
    edge = enter[strong.find(edge.from)];
  }
  return result;
}

std::vector<WeightedEdge> arborescence(const std::set<WeightedEdge>& E, std::size_t& arbor_root, std::size_t n) {
  DynArray<std::vector<WeightedEdge>> ingoing_queue(n);
  DynArray<TwoLevelHeap> ingoing(n);
  UnionFindNoWeight strong(n);
  UnionFindNoWeight weak(n);
  DynArray<WeightedEdge> enter(n);
  DynArray<std::size_t> enter_num(n);
  DynArray<Cycle> cycle_forming_strong(n);
  DynArray<std::size_t> max(n);
  std::queue<std::size_t> roots;
  for(std::size_t i = 0; i < n; ++i) {
    enter[i] = WeightedEdge(END, END, 0);
    enter_num[i] = END;
    ingoing_queue[i] = std::vector<WeightedEdge>();
    cycle_forming_strong[i] = Cycle();
    roots.push(i);
    max[i] = i;
  }
  for(const auto& e: E) {
    ingoing_queue[e.to].push_back(e);
  }
  for(std::size_t i = 0; i < n; ++i) {
    std::sort(ingoing_queue[i].rbegin(), ingoing_queue[i].rend());
    ingoing[i] = TwoLevelHeap(std::move(ingoing_queue[i]));
  }

  ForestResult F(n);
  std::vector<std::size_t> H_roots;

  while(!roots.empty()) {
    auto root = roots.front();
    roots.pop();
    
    assert(strong.find(root) == root);
    assert(enter[root].from == END);

    if(ingoing[root].empty()) {
      H_roots.push_back(root);
      continue;
    }
    auto edge = ingoing[root].top();
    ingoing[root].pop();

    if(strong.find(edge.from) == strong.find(root)) {
      roots.push(root);
      continue;
    }

    auto node_num = F.add_node(Edge(edge.from, edge.to));
    for(auto num: cycle_forming_strong[root].edge_nums) {
      F.add_child(node_num, num);
    }

    if(weak.union_sets(edge.from, root)) {
      enter[root] = edge;
      enter_num[root] = node_num;
      continue;
    }

    auto cycle = strong_path(enter, enter_num, strong, edge, node_num);
    
    max[root] = max[strong.find(cycle.heavy.to)];
    ingoing[root].offset(cycle.heavy.weight - edge.weight);
    for(std::size_t i = 1; i < cycle.edges.size(); ++i) {
      auto strong_edge = cycle.edges[i];
      auto component = cycle.components[i];
      ingoing[component].offset(cycle.heavy.weight - strong_edge.weight);
      ingoing[root].join(std::move(ingoing[component]));
      strong.union_sets(root, component);
    }

    cycle_forming_strong[root] = cycle;

    assert(root == strong.find(root));
    roots.push(root);
  }

  assert(H_roots.size() == 1);

  arbor_root = max[H_roots[0]];
  auto edge = F.nodes[arbor_root].created_from;
  F.delete_representative_to_root_path(edge.to);

  std::set<Edge> final_edges;
  auto last_root = END;
  while(F.roots.size() > 0) {
    auto result_root = *F.roots.begin();
    if(result_root == last_root) {
      return {};
    }
    last_root = result_root;
    auto edge = F.nodes[result_root].created_from;
    final_edges.insert(edge);
    F.delete_representative_to_root_path(edge.to);
  }

  auto comp_lambda = [](const WeightedEdge& lhs, const WeightedEdge& rhs) {
    return std::tie(lhs.from, lhs.to, lhs.weight) < std::tie(rhs.from, rhs.to, rhs.weight);
  };

  std::vector<WeightedEdge> lookup(E.begin(), E.end());
  sort(lookup.begin(), lookup.end(), comp_lambda);
  std::vector<WeightedEdge> mapped_result;
  for(const auto& e: final_edges) {
    mapped_result.push_back(*std::lower_bound(lookup.begin(), lookup.end(), WeightedEdge(e.from, e.to, 0), comp_lambda));
  }
  return mapped_result;
}
