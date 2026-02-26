#include<algorithm>
#include<utility>
#include<vector>
#include<cmath>
#include<iostream>

#include "hrlz/file.hpp"
#include "hrlz/ArrayHash.hpp"
#include "hrlz/Fingerprint.hpp"
#include "hrlz/Cluster.hpp"

const std::size_t SHINGLE_SIZE = 256;
const std::size_t FINGERPRINT_SIZE = 4;

std::size_t cluster_threshold(std::size_t n) {
  return 2 * std::sqrt(n);
}

void report_edges(const AdjacencyList& graph, const DynArray<std::filesystem::path>& files) {
  std::cout << graph.edge_count() << std::endl;
  for(std::size_t i = 0; i < graph.node_count(); ++i) {
    for(std::size_t j = 0; j < graph.node_count(); ++j) {
      if(graph.has_edge(i,j)) {
        std::cout << files[i] << " " << files[j] << std::endl;
      }
    }
  }
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  ClusterSettings cs;
  cs.files = read_input_file(argv[1]);
  cs.shingle_size = SHINGLE_SIZE;
  cs.fingerprint_size = FINGERPRINT_SIZE;
  cs.cutoff = cluster_threshold(cs.files.size());
  auto g = compute(cs);
  std::unique_ptr<AdjacencyList> adj = g->take_adjacencyList();
  report_edges(*adj, cs.files);
}
