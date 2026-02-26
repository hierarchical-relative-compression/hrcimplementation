#pragma once
#include "common/DynArray.hpp"

class UnionFind {
public:
  UnionFind(): _ids(0), _size(0), _sets(0) {}
  UnionFind(std::size_t size): _ids(size), _size(size), _sets(size) {
    for(std::size_t i = 0; i < _ids.size(); ++i) {
      _ids[i] = i;
      _size[i] = 1;
    }
  }
  std::size_t find(std::size_t i) {
    std::size_t root = i;
    while(_ids[root] != root) {
      root = _ids[root];
    }
    std::size_t next = _ids[i];
    while(next != root) {
      _ids[i] = root;
      i = next;
      next = _ids[next];
    }
    return root;
  }

  std::size_t size(std::size_t i) {
    return _size[find(i)];
  }
  std::size_t sets() {
    return _sets;
  }
  bool union_sets(std::size_t i, std::size_t j) {
    i = find(i);
    j = find(j);
    if(i != j) {
      _sets -= 1;
      if(_size[i] > _size[j]) {
        _ids[j] = i;
        _size[i] += _size[j];
      }
      else {
        _ids[i] = j;
        _size[j] += _size[i];
      }
      return true;
    }
    return false;
  }
private:
  DynArray<std::size_t> _ids;
  DynArray<std::size_t> _size;
  std::size_t _sets;
};

// If we need to guarantee that representatitve is maintained
// k = find(i);
// union(i,j);
// k == find(j);
class UnionFindNoWeight {
public:
  UnionFindNoWeight(): _ids(0), _sets(0) {}
  UnionFindNoWeight(std::size_t size): _ids(size), _size(size), _sets(size) {
    for(std::size_t i = 0; i < _ids.size(); ++i) {
      _ids[i] = i;
      _size[i] = 1;
    }
  }
  std::size_t find(std::size_t i) {
    std::size_t root = i;
    while(_ids[root] != root) {
      root = _ids[root];
    }
    std::size_t next = _ids[i];
    while(next != root) {
      _ids[i] = root;
      i = next;
      next = _ids[next];
    }
    return root;
  }
  std::size_t sets() {
    return _sets;
  }
  bool union_sets(std::size_t i, std::size_t j) {
    i = find(i);
    j = find(j);
    if(i != j) {
      _sets -= 1;
      _ids[j] = i;
      _size[i] += _size[j];
      _size[j] = 0;
      return true;
    }
    return false;
  }

  std::size_t size(std::size_t i) {
    return _size[find(i)];
  }
private:
  DynArray<std::size_t> _ids;
  DynArray<std::size_t> _size;
  std::size_t _sets;
};
