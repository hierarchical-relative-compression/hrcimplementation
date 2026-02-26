#pragma once
#include<vector>
#include<map>
#include<set>

struct Edge {
  std::size_t from, to;
  void swap(Edge& other) {
    std::swap(from, other.from);
    std::swap(to, other.to);
  }
  bool operator<(const Edge& rhs) const {
    return std::tie(from, to) < std::tie(rhs.from, rhs.to);
  }
};

struct WeightedEdge {
  std::size_t from, to;
  float weight;
  void swap(WeightedEdge& other) {
    std::swap(from, other.from);
    std::swap(to, other.to);
    std::swap(weight, other.weight);
  }
  bool operator<(const WeightedEdge& rhs) const {
    return std::tie(weight, from, to) < std::tie(rhs.weight, rhs.from, rhs.to);
  }
};

std::vector<WeightedEdge> arborescence(const std::set<WeightedEdge>& E, std::size_t& root, std::size_t n);
