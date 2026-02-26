//
// Created by mpelo on 5/30/24.
//

#include "hrlz/Cluster.hpp"
#include <iostream>

int test_empty_graph() {
    /*
     * First easy test
     */
    /*
     * Setup
     */
    ClusterGraph g(9);
    ClusterSettings cs;
    cs.cutoff = 5;
    std::vector<std::size_t> cluster = {4, 5, 6, 7, 8};
    DynArray<std::size_t> collisions(9);
    /*
     * Run
     */
    handle_cluster(cs, cluster, g, collisions);
    /*
     * Check
     */
    std::unique_ptr<AdjacencyList> adj = g.take_adjacencyList();
    for (int i = 4; i <= 8; i++) {
        for (int j = 4; j <= 8; j++) {
            if (i != j) {
                bool exists = adj->has_edge(i, j);
                if (!exists) {
                    std::cerr << "Edge " << i << ", " << j << " does not exist" << std::endl;
                    return 1;
                }
            }
        }
    }
    std::cout << "Test correct" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return test_empty_graph();
}

