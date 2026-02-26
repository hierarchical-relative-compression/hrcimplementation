#pragma once
#include "hrlz/UnionFind.hpp"
#include "hrlz/AdjacencyList.hpp"
#include "common/DynArray.hpp"
#include "hrlz/Arborescence.hpp"

#include<filesystem>
#include<set>

class ClusterGraph {
public:
  ClusterGraph(std::size_t node_count): _adj(std::make_unique<AdjacencyList>(node_count)), _components(node_count) {}
  void connect_cluster(const std::vector<std::size_t>& cluster) {
    for(std::size_t i = 0; i < cluster.size(); ++i) {
      for(std::size_t j = i + 1; j < cluster.size(); ++j) {
        auto ic = cluster[i];
        auto jc = cluster[j];
        _adj->add_edge(ic,jc);
        _adj->add_edge(jc,ic);
        _components.union_sets(ic,jc);
      }
    }
  }
  std::size_t number_of_components() {
    return _components.sets();
  }
  bool fully_connected() {
    return _components.sets() == 1;
  }
  std::unique_ptr<AdjacencyList> take_adjacencyList() {
    return std::move(_adj);
  }
  std::size_t find(std::size_t i) {
    return _components.find(i);
  }
private:
  std::unique_ptr<AdjacencyList> _adj;
  UnionFind _components;
};

struct ClusterSettings {
  DynArray<std::filesystem::path> files;
  std::size_t shingle_size;
  std::size_t fingerprint_size;
  std::size_t cutoff;
  std::size_t rounds_until_shrink;
  bool bottom_k;
};

ClusterGraph *compute(ClusterSettings& cs);
void handle_cluster(const ClusterSettings &cs, const std::vector<std::size_t> &cluster, ClusterGraph &g, DynArray<std::size_t> &collisions);
std::unique_ptr<std::set<WeightedEdge>> compute_rlz_for_edges(const AdjacencyList& adj_list, const DynArray<std::filesystem::path>& files, const DynArray<std::filesystem::path>& sa_files);
std::unique_ptr<std::set<WeightedEdge>> compute_approx_weights(const AdjacencyList &adj_list, const DynArray<std::filesystem::path> &files, size_t shingle_size, size_t fingerprint_size);