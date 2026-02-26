#include<hrlz/BFS.hpp>
#include<common/DynArray.hpp>
#include<queue>


std::vector<Edge> bfs_order_edges(const std::vector<WeightedEdge>& edges, const std::size_t n, const std::size_t root) {
  std::vector<Edge> result;
  result.reserve(edges.size());
  DynArray<std::vector<std::pair<std::size_t, std::size_t>>> adj_list(n);
  for(std::size_t i = 0; i < n; ++i) {
    adj_list[i] = std::vector<std::pair<std::size_t, std::size_t>>();
  }
  for(const auto& edge: edges){
    adj_list[edge.from].emplace_back(edge.to, edge.weight);
  }
  std::queue<std::size_t> q;
  q.push(root);
  while(q.size() > 0) {
    auto curr = q.front();
    q.pop();
    for(const auto& adj: adj_list[curr]) {
      result.emplace_back(curr, adj.first);
      q.push(adj.first);
    }
  }
  return result;
}

