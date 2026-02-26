#include<vector>
#include "Arborescence.hpp"

// Orders edges from an arborescence such that the edges are ordered by the visit time in a bfs traversal
std::vector<Edge> bfs_order_edges(const std::vector<WeightedEdge>& edges, const std::size_t n, const std::size_t root); 
