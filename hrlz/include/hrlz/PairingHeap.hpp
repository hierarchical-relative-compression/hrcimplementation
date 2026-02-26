#pragma once
#include<vector>
#include<optional>
#include<functional>

template<class T, class O> 
struct DefaultOffsetter {
  T operator()(const T& v, const O&) {
    return v;
  }
};

struct DefaultOffset {
  DefaultOffset& operator+=(const DefaultOffset&) {
    return *this;
  }

  friend DefaultOffset operator+(DefaultOffset& lhs, const DefaultOffset&) {
    return lhs;
  }

  friend DefaultOffset operator-(DefaultOffset& lhs, const DefaultOffset&) {
    return lhs;
  }

  friend DefaultOffset operator-(DefaultOffset& lhs) {
    return lhs;
  }
};

template<
    class T,
    class Compare = std::less<T>,
    class O = DefaultOffset,
    class Offsetter = DefaultOffsetter<T, O>
> class PairingHeap;
template<
    class T,
    class Compare,
    class O,
    class Offsetter
> class PairingHeap {
public:
  PairingHeap(): _offset(), _elem(), _subheaps() {}
  PairingHeap(T v): _offset(), _elem(v), _subheaps() {}
  PairingHeap(PairingHeap&& moved) = default;
  PairingHeap& operator=(PairingHeap&& moved) = default;
  bool empty() const {
    return !_elem.has_value();
  }
  const T top() const {
    return Offsetter()(_elem.value(), _offset);
  }
  void pop() {
    _elem.reset();
    auto tmp = std::move(_subheaps);
    if(tmp.size() == 0 ){
      _offset = O();
    }
    auto old_offset = _offset;
    for(auto& h: tmp) {
      h.offset(old_offset);
      join(std::move(h));
    }
  }
  void push(T v) {
    PairingHeap h(v);
    join(std::move(h));
  }
  void join(PairingHeap&& h) {
    if(h.empty())
      return;
    if(empty()) {
      *this = std::move(h);
      return;
    }
    if (!Compare()(top(), h.top())){
      std::swap(*this, h);
    }
    h.offset(-_offset);
    _subheaps.push_back(std::move(h));
  }
  void offset(const O& o) {
    _offset += o;
  }
private:
  O _offset;
  std::optional<T> _elem;
  std::vector<PairingHeap> _subheaps;
};
