#pragma once
#include<unordered_set>
#include<vector>

#include "common/DynArray.hpp"
#include "hrlz/Arborescence.hpp"

class AdjacencyList {
public:
  AdjacencyList() {}
  AdjacencyList(std::size_t n): _adj(n), _n(n), _edge_count(0) {
  }
  bool has_edge(std::size_t i, std::size_t j) const noexcept {
    return _adj[i].contains(j);
  }
  void add_edge(std::size_t i, std::size_t j) noexcept {
    auto old_size = _adj[i].size();
    _adj[i].insert(j);
    _edge_count += _adj[i].size() - old_size;
  }
  std::size_t node_count() const noexcept {
    return _n;
  }
  std::size_t edge_count() const noexcept {
    return _edge_count;
  }
  const std::unordered_set<std::size_t>& adjacent(std::size_t i) const {
    return _adj[i];
  }
  void reserve_for_i(std::size_t i, std::size_t size) {
      _adj[i].reserve(size);
  }
private:
  DynArray<std::unordered_set<std::size_t>> _adj;
  std::size_t _n;
  std::size_t _edge_count;
};
